#ifndef __REGISTER__
#define __REGISTER__

#include "register_config.hpp"
#include "SCMUlate_tools.hpp"
#include <string>
#include <iostream>

namespace scm {
  typedef union {
    // the 1000 is because the register file size is in KB
    char space[REG_FILE_SIZE_KB*1000];
    registers_t registers;
  } register_file_t;

  class reg_file_module {
    private: 
      register_file_t *reg_file;
    
    public: 
     reg_file_module();
     void describeRegisterFile();
     bool checkRegisterConfig();
     inline char * getRegisterByName(std::string size, int num) {
       char * result = NULL;
       if (size == "64B")
         result = REG_64B(num).data;
       else if (size == "1L")
         result = REG_1L(num).data;
       else if (size == "8L")
         result = REG_8L(num).data;
       else if (size == "16L")
         result = REG_16L(num).data;
       else if (size == "256L")
         result = REG_256L(num).data;
       else if (size == "512L")
         result = REG_512L(num).data;
       else if (size == "1024L")
         result = REG_1024L(num).data;
       else if (size == "2048L")
         result = REG_2048L(num).data;
       else
         SCMULATE_ERROR(0, "DECODED REGISTER DOES NOT EXIST!!!")
       return result;
     };
     void dumpRegisterFile();
     void dumpRegister(std::string size, int num);
     ~reg_file_module();
  };
}

#endif
