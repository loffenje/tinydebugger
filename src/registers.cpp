
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <algorithm>
#include <string>

#include "registers.h"

using namespace tinydebugger;


std::string tinydebugger::getRegisterName(reg r) 
{
    auto it = std::find_if(std::begin(gRegisterDescriptors), std::end(gRegisterDescriptors),
		    [r](auto&& rd) { return rd.r == r; });

    return it->name;
}

reg tinydebugger::getRegisterFromName(const std::string &name)
{
    auto it = std::find_if(std::begin(gRegisterDescriptors), std::end(gRegisterDescriptors),
		    [name](auto&& rd) { return rd.name == name; });

    return it->r;
}

uint64_t tinydebugger::getRegisterValue(pid_t pid, reg r) 
{
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    
    auto it = std::find_if(std::begin(gRegisterDescriptors), std::end(gRegisterDescriptors),
		    [r](auto&& rd) { return rd.r == r; });

    // instead of big switch stmt. 
    return *(reinterpret_cast<uint64_t *>(&regs) + (it - std::begin(gRegisterDescriptors)));
}

void tinydebugger::setRegisterValue(pid_t pid, reg r, uint64_t value) 
{
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    
    auto it = std::find_if(std::begin(gRegisterDescriptors), std::end(gRegisterDescriptors),
		    [r](auto&& rd) { return rd.r == r; });

    *(reinterpret_cast<uint64_t *>(&regs) + (it - std::begin(gRegisterDescriptors))) = value;
    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

uint64_t tinydebugger::getRegisterValueFromDwarf(pid_t pid, unsigned int regnum) 
{
    auto it = std::find_if(std::begin(gRegisterDescriptors), std::end(gRegisterDescriptors),
		    [regnum](auto&& rd) { return rd.dwarfR == regnum; });
    if (it == std::end(gRegisterDescriptors)) {
	throw std::out_of_range{"Unknown dwarf register"};
    }

    return getRegisterValue(pid, it->r);
}

