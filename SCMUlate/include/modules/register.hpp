#ifndef __REGISTER__
#define __REGISTER__

#include "register_config.hpp"
#include "SCMUlate_tools.hpp"
#include <string>
#include <iostream>

namespace scm {
  typedef union {
    // the 1000 is because the register file size is in KB
    unsigned char space[REG_FILE_SIZE_KB*1000];
    registers_t registers;
  } register_file_t;

  class reg_file_module {
    private: 
      register_file_t *reg_file;
    
    public: 
     reg_file_module();
     void describeRegisterFile();
     bool checkRegisterConfig();
     static inline uint32_t getRegisterSizeInBytes(std::string size) {
       uint32_t result = 0;
       if (size == "64B")
         result = 8;
       else if (size == "1L")
         result = CACHE_LINE_SIZE;
       else if (size == "8L")
         result = CACHE_LINE_SIZE*8;
       else if (size == "16L")
         result = CACHE_LINE_SIZE*16;
       else if (size == "256L")
         result = CACHE_LINE_SIZE*256;
       else if (size == "512L")
         result = CACHE_LINE_SIZE*512;
       else if (size == "1024L")
         result = CACHE_LINE_SIZE*1024;
       else if (size == "2048L")
         result = CACHE_LINE_SIZE*2048;
       else
         SCMULATE_ERROR(0, "DECODED REGISTER DOES NOT EXIST!!!")
       return result;
     };
     inline unsigned char * getRegisterByName(std::string size, int num) {
       unsigned char * result = NULL;
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
     // Function used for renaming provides the next register but from the end of the register file
     // This is to avoid coliding with nearby registers hopefully saving time in renaming process
     // It modifies the num by reference. Resulting in the new register number
     inline unsigned char * getNextRegister(std::string size, uint32_t & num) {
       unsigned char * result = NULL;
       if (size == "64B") {
         num = (num + 1) % NUM_REG_64BITS;
         result = REG_64B(num).data;
       } else if (size == "1L") {
         num = (num + 1) % NUM_REG_1LINE;
         result = REG_1L(num).data;
       } else if (size == "8L") {
         num = (num + 1) % NUM_REG_8LINE;
         result = REG_8L(num).data;
       } else if (size == "16L") {
         num = (num + 1) % NUM_REG_16LINE;
         result = REG_16L(num).data;
       } else if (size == "256L") {
         num = (num + 1) % NUM_REG_256LINE;
         result = REG_256L(num).data;
       } else if (size == "512L") {
         num = (num + 1) % NUM_REG_512LINE;
         result = REG_512L(num).data;
       } else if (size == "1024L") {
         num = (num + 1) % NUM_REG_1024LINE;
         result = REG_1024L(num).data;
       } else if (size == "2048L") {
         num = (num + 1) % NUM_REG_2048LINE;
         result = REG_2048L(num).data;
       } else
         SCMULATE_ERROR(0, "DECODED REGISTER DOES NOT EXIST!!!")
       return result;

     }
     void dumpRegisterFile();
     void dumpRegister(std::string size, int num);
     ~reg_file_module();
  };
}

#endif
