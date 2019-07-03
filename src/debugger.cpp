
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "debugger.h"
#include "registers.h"
#include "linenoise.h"

using namespace tinydebugger;

std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> ret{};
    std::stringstream ss{s};
    std::string item;
    while (std::getline(ss, item, delimiter)) {
	ret.emplace_back(item);
    }

    return ret;
}

bool isPrefix(const std::string &s, const std::string &of)
{
    if (s.size() > of.size()) return false;

    return std::equal(s.begin(), s.end(), of.begin());
}


uint64_t Debugger::getPC() 
{
    return getRegisterValue(m_pid, reg::rip);
}

void Debugger::setPC(uint64_t pc) 
{
    setRegisterValue(m_pid, reg::rip, pc);
}


void Debugger::handleSigtrap(siginfo_t info) 
{
    switch (info.si_code) {
	// these will be sent when a breakpoint is hit
        case SI_KERNEL:
	case TRAP_BRKPT: 
	{
	    setPC(getPC() - 1);
	    std::cout << "Hit breakpoint at address 0x" << std::hex << getPC() << '\n';
	    auto lineEntry = getLineEntryFromPC(getPC());
	    printSource(lineEntry->file->path, lineEntry->line);
	    return;
	}
	// this will be sent if the signal was sent by signal stepping
	case TRAP_TRACE:
	    return;
	default:
	    std::cout << "Unknown SIGTRAP code " << info.si_code << '\n';
	    return;
    }
}

void Debugger::waitForSignal()
{
    int waitStatus;
    auto options = 0;
    waitpid(m_pid, &waitStatus, options);

    auto siginfo = getSignalInfo();
    
    switch (siginfo.si_signo) {
	case SIGTRAP:
	    handleSigtrap(siginfo);
	    break;
	case SIGSEGV:
	    std::cout << "SEGFAULT. Code: " << siginfo.si_code << std::endl;
	    break;
	default:
	    std::cout << "Got signal " << strsignal(siginfo.si_signo) << std::endl;
    }
}


void Debugger::stepOverBreakpoint()
{
    if (m_breakpoints.count(getPC())) {
        auto &bp = m_breakpoints[getPC()];
	if (bp.isEnabled()) {
	   bp.disable();
	   ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
	   waitForSignal();
	   bp.enable();  
	}
    }
}

void Debugger::run() 
{
    int waitStatus;
    auto options = 0;
    waitpid(m_pid, &waitStatus, options);

    char *line = nullptr;
    while ((line = linenoise("td> ")) != nullptr) {
        handleCommand(line);
	linenoiseHistoryAdd(line);
	linenoiseFree(line);
    }
}

void Debugger::handleCommand(const std::string &line) 
{
    auto args = split(line, ' ');
    auto command = args[0];
    if (isPrefix(command, "cont")) {
	continueExecution();
    } else if(isPrefix(command, "register")) {
    	dumpRegisters();
    } else if(isPrefix(args[1], "read")) {
    	std::cout << getRegisterValue(m_pid, getRegisterFromName(args[2])) << std::endl;
    } else if(isPrefix(args[1], "write")) {
    	std::string val{args[3], 2}; // e.g. 0xVAL
	setRegisterValue(m_pid, getRegisterFromName(args[2]), std::stol(val, 0, 16));
    } else if (isPrefix(command, "break")) {
	std::string addr {args[1], 2};
	setBreakpointAtAddress(std::stol(addr, 0, 16));
    } else if(isPrefix(command, "memory")) {
    	std::string addr{args[2], 2}; // e.g. 0xADDRESS
	if (isPrefix(args[1], "read")) {
	    std::cout << std::hex << readMemory(std::stol(addr, 0, 16)) << std::endl;
	}

	if (isPrefix(args[1], "write")) {
	    std::string val {args[3], 2}; // e.g. 0xVAL
	    writeMemory(std::stol(addr, 0, 16), std::stol(val, 0, 16));
	}
    } else {
	std::cerr << "Unknown command\n";
    }
}

void Debugger::continueExecution() 
{
    stepOverBreakpoint();
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    waitForSignal();
}

void Debugger::setBreakpointAtAddress(std::intptr_t addr)
{
    std::cout << "Set breakpoint at address 0x" << std::hex << addr << '\n';
    Breakpoint bp{m_pid, addr};
    bp.enable();
    m_breakpoints[addr] = bp;
}

uint64_t Debugger::readMemory(uint64_t address) 
{
    return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void Debugger::writeMemory(uint64_t address, uint64_t value) 
{
    ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

dwarf::die Debugger::getFuncFromPC(uint64_t pc)
{
    for (auto &cu: m_dwarf.compilation_units()) {
	if (die_pc_range(cu.root()).contains(pc)) {
	    for (const auto &die: cu.root()) {
		if (die.tag == dwarf::DW_TAG::subprogram) {
		    if (die_pc_range(die).contains(pc)) {
			return die;
		    }
		}
	    }
	}
    }

    throw std::out_of_range{"Cannot find a function"};
}

dwarf::line_table::iterator Debugger::getLineEntryFromPC(uint64_t pc)
{
    for (auto &cu: m_dwarf.compilation_units()) {
	if (die_pc_range(cu.root()).contains(pc)) {
	    auto &lt = cu.get_line_table();
	    auto it = lt.find_address(pc);
	    if (it == lt.end()) {
		throw std::out_of_range{"Cannot find line entry"};
	    } else {
		return it;
	    }
	}
    }

    throw std::out_of_range{"Cannot find line entry"};
}


void Debugger::printSource(const std::string &fileName, unsigned int line, unsigned int nlinesContext) 
{
    std::ifstream file{fileName};

    auto startLine = line <= nlinesContext ? 1 : line - nlinesContext;
    auto endLine = line + nlinesContext + (line < nlinesContext ? nlinesContext - line : 0) + 1;

    char c{};
    auto currentLine = 1U;
    while (currentLine != startLine && file.get(c)) {
	if (c == '\n') {
	    ++currentLine;
	}
    }

    std::cout << (currentLine == line ? "> " : " ");

    while (currentLine <= endLine && file.get(c)) {
        std::cout << c;
	if (c == '\n') {
	    ++currentLine;

	    std::cout << (currentLine == line ? "> " : " ");
	}
    }

    std::cout << std::endl;
}

siginfo_t Debugger::getSignalInfo()
{
    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, m_pid, nullptr, &info);

    return info;
}

void Debugger::dumpRegisters() 
{
    for (const auto &rd : gRegisterDescriptors) {
	std::cout << rd.name << "0x" 
		<< std::setfill('0') << std::setw(16) << std::hex << getRegisterValue(m_pid, rd.r) 
		<< '\n';
    }
}
