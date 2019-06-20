#include <iostream>
#include <unistd.h>
#include <sys/ptrace.h>

#include "debugger.h"

using namespace tinydebugger;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Program name not specified\n";
	return -1;
    }

    auto prog = argv[1];
    auto pid = fork();
    if (pid == 0) {
	// child process
	ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
	execl(prog, prog, nullptr);
    } else if (pid >= 1) {
	// parent process
	Debugger dbg{prog, pid};
	dbg.run();
    }
}
