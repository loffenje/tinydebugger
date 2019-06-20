
#include <sys/ptrace.h>

#include "breakpoint.h"

using namespace tinydebugger;

void Breakpoint::enable() 
{
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    m_savedData = static_cast<uint8_t>(data & 0xff); // save bottom type. The address of breakpoint
    uint64_t int3 = 0xcc; // int instruction
    uint64_t dataWithInt3 = ((data & ~0xff) | int3); // set bottom byte to 0xcc
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, dataWithInt3);

    m_enabled = true;
}


void Breakpoint::disable()
{
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    auto restoredData = ((data & ~0xff) | m_savedData);
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, restoredData);

   m_enabled = false; 
}
