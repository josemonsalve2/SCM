#include "instruction_mem.hpp"

int main () {
  scm::inst_mem_module mem_from_file("test_mem_file.txt");
  mem_from_file.dumpMemory();
  //scm::inst_mem_module mem_from_stdin("");
  //mem_from_stdin.dumpMemory();

  return 0;
}
