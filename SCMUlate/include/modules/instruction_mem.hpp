#ifndef __INST_MEM__
#define __INST_MEM__

#include "SCMUlate_tools.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

namespace scm {
  class inst_mem_module {
    private: 
      std::vector<std::string> memory;
      bool is_valid; 

    public:
      inst_mem_module() = delete;
      inst_mem_module(std::string const filename);
  
      inline std::string fetch(int address) { return memory[address]; };
      void dumpMemory();
  };
}

#endif
