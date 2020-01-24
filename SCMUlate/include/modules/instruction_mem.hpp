#ifndef __INST_MEM__
#define __INST_MEM__

#include "SCMUlate_tools.hpp"
#include "instructions.hpp"
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>

namespace scm {
  /* This module corresponds to the instruction memory, it is in charge 
   * of reading the file that contains the program, or read the program 
   * directly from the user input. It uses the instructions class to 
   * identify if the readed line is an instruction, otherwise it will try
   * to map it to a label, if that does not work it will ignore the line
   * and raise a warning. Additionally, it is possible to fetch any instruction
   * by passing the corresponding address
   *
   */
  class inst_mem_module {
    private: 
      std::vector<std::string> memory;
      std::map<std::string, int> labels;


      /* This flag checks if the file that is received is a valid flag
       * otherwise it should set it to stop running the machine
       */
      bool is_valid; 

      /* This method parses the file that is sent containing the program
       * and load all the insturctions and labels into the instruction memory
       * If the program is invalid it will return false. If the program is 
       * valid it will return true
       */
      bool loader(std::string const filename);

    public:
      inst_mem_module() = delete;
      inst_mem_module(std::string const filename);
  
      /* This method allows to fetch an instruction from the instruction 
       * memory by passing the address (PC)
       */
      inline std::string fetch(int address) { return memory[address]; };

      /* This method allows to dump the content of the instruction 
       * memory for debugging
       */
      void dumpMemory();
  };
}

#endif
