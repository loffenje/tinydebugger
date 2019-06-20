
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "debugger.h"
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
    } else if (isPrefix(command, "break")) {
	std::string addr {args[1], 2};
	setBreakpointAtAddress(std::stol(addr, 0, 16));
    } else {
	std::cerr << "Unknown command\n";
    }
}

void Debugger::continueExecution() 
{
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);

    int waitStatus;
    auto options = 0;
    waitpid(m_pid, &waitStatus, options);
}

void Debugger::setBreakpointAtAddress(std::intptr_t addr)
{
    std::cout << "Set breakpoint at address 0x" << std::hex << addr << '\n';
    Breakpoint bp{m_pid, addr};
    bp.enable();
    m_breakpoints[addr] = bp;
}

