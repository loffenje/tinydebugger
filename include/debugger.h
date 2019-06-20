#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>
#include <utility>
#include <sys/types.h>
#include <unordered_map>

#include "breakpoint.h"

namespace tinydebugger {

    class Debugger {
    public:
	Debugger(std::string progName, pid_t pid) : m_progName{std::move(progName)}, m_pid{pid} {}

	void run();

	void setBreakpointAtAddress(std::intptr_t addr);

    private:
	void handleCommand(const std::string &line);
	void continueExecution();

    private:
	std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
	std::string m_progName;
	pid_t m_pid;
    }; 

}
#endif
