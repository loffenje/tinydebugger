
#include <vector>
#include <sstream>
#include <iostream>
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

void Debugger::waitForSignal()
{
    int waitStatus;
    auto options = 0;
    waitpid(m_pid, &waitStatus, options);
}

void Debugger::stepOverBreakpoint()
{
   // -1 because execution will go past the breakpoint
    auto possibleBreakpointLoc = getPC() - 1;
    if (m_breakpoints.count(possibleBreakpointLoc)) {
        auto &bp = m_breakpoints[possibleBreakpointLoc];
	if (bp.isEnabled()) {
	   auto previousInstrAddr = possibleBreakpointLoc;
	   setPC(previousInstrAddr);
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

void Debugger::dumpRegisters() 
{
    for (const auto &rd : gRegisterDescriptors) {
	std::cout << rd.name << "0x" 
		<< std::setfill('0') << std::setw(16) << std::hex << getRegisterValue(m_pid, rd.r) 
		<< '\n';
    }
}
