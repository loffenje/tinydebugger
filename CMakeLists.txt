cmake_minimum_required (VERSION 3.0)
project (tinydebugger)

add_compile_options(-std=c++14)

include_directories(ext/libelfin ext/linenoise include)
add_executable(tinydebugger src/main.cpp src/breakpoint.cpp src/registers.cpp src/debugger.cpp ext/linenoise/linenoise.c)


add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/ext/libelfin
)
target_link_libraries(tinydebugger
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_SOURCE_DIR}/ext/libelfin/elf/libelf++.so)
add_dependencies(tinydebugger libelfin)
