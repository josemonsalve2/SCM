#ifndef __REGISTER__
#define __REGISTER__

#include "register_config.hpp"
#include "SCMUlate_tools.hpp"

namespace scm {
  typedef union {
    // the 1000 is because the register file size is in KB
    char space[REG_FILE_SIZE_KB*1000];
    registers_t registers;
  } register_file_t;

  class reg_file_module {
    private: 
      register_file_t reg_file;
    
    public: 
     reg_file_module();
     void describeRegisterFile();
     bool checkRegisterConfig();
     char * getRegisterByName(char * size, int num);
  };
}

#endif
