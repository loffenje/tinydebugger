#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <cstdint>
#include <sys/types.h>

namespace tinydebugger {
    class Breakpoint {
    public:
	Breakpoint() = default;
	Breakpoint(pid_t pid, std::intptr_t addr) : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_savedData{} {}

	void enable();
	void disable();

	auto isEnabled() const -> bool { return m_enabled; }
	auto getAddress() const -> std::intptr_t { return m_addr; }
    private:
	pid_t m_pid;
	std::intptr_t m_addr;
	uint8_t m_savedData;
	bool m_enabled;
    };
}

#endif
