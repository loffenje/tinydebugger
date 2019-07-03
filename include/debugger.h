#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>
#include <utility>
#include <sys/types.h>
#include <fcntl.h>
#include <unordered_map>
#include <signal.h>

#include "breakpoint.h"
#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"

namespace tinydebugger {

    class Debugger {
    public:
	Debugger(std::string progName, pid_t pid) : m_progName{std::move(progName)}, m_pid{pid} {
	    auto fd = open(m_progName.c_str(), O_RDONLY);

	    m_elf = elf::elf{elf::create_mmap_loader(fd)};
	    m_dwarf = dwarf::dwarf{dwarf::elf::create_loader(m_elf)};
	
	}

	void run();

	void setBreakpointAtAddress(std::intptr_t addr);

	void dumpRegisters();

	void stepOverBreakpoint();

	dwarf::die getFuncFromPC(uint64_t pc);

	dwarf::line_table::iterator getLineEntryFromPC(uint64_t pc);

	void printSource(const std::string &fileName, unsigned int line, unsigned int nlinesContext = 2);

    private:
	void handleCommand(const std::string &line);
	
	void continueExecution();

	uint64_t readMemory(uint64_t address);
	
	void writeMemory(uint64_t address, uint64_t value);

	uint64_t getPC();

	void setPC(uint64_t pc);

	void waitForSignal();

	siginfo_t getSignalInfo();

	void handleSigtrap(siginfo_t info);

    private:
	std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
	std::string m_progName;
	pid_t m_pid;
	dwarf::dwarf m_dwarf;
	elf::elf m_elf;
    }; 

}

#endif
