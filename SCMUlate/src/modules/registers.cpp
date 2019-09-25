#include "register.hpp"
#include <sstream>
#include <iomanip>

scm::reg_file_module::reg_file_module() {
  SCMULATE_INFOMSG(3, "Initializing Register file");
  reg_file = new register_file_t();
  // Initialize the memory that represents the register
  // file to zero
  for (int i = 0; i < REG_FILE_SIZE_KB*1000 ; ++ i) {
    reg_file->space[i] = 0;
  }
  this->describeRegisterFile();
  this->checkRegisterConfig();
}

void 
scm::reg_file_module::describeRegisterFile() {
  SCMULATE_INFOMSG(0, "REGISTER FILE DEFINITION");
  SCMULATE_INFOMSG(0, " SIZE = %ld", CALCULATE_REG_SIZE);
  SCMULATE_INFOMSG(1, " %ld registers of 64BITS, each of %d bytes. Total size = %ld  -- %f percent ", NUM_REG_64BITS, 64/8, NUM_REG_64BITS*64/8, (NUM_REG_64BITS*64/8)*100.0f/CALCULATE_REG_SIZE);
  SCMULATE_INFOMSG(1, " %ld registers of 1LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_1LINE, CACHE_LINE_SIZE, NUM_REG_1LINE*CACHE_LINE_SIZE, NUM_REG_1LINE*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
  SCMULATE_INFOMSG(1, " %ld registers of 8LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_8LINE, CACHE_LINE_SIZE*8, NUM_REG_8LINE*8*CACHE_LINE_SIZE, NUM_REG_8LINE*8*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
  SCMULATE_INFOMSG(1, " %ld registers of 16LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_16LINE, CACHE_LINE_SIZE*16, NUM_REG_16LINE*16*CACHE_LINE_SIZE, NUM_REG_16LINE*16*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
  SCMULATE_INFOMSG(1, " %ld registers of 256LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_256LINE, CACHE_LINE_SIZE*256, NUM_REG_256LINE*256*CACHE_LINE_SIZE, NUM_REG_256LINE*256*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
  SCMULATE_INFOMSG(1, " %ld registers of 512LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_512LINE, CACHE_LINE_SIZE*512, NUM_REG_512LINE*512*CACHE_LINE_SIZE, NUM_REG_512LINE*512*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
  SCMULATE_INFOMSG(1, " %ld registers of 1024LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_1024LINE, CACHE_LINE_SIZE*1024, NUM_REG_1024LINE*1024*CACHE_LINE_SIZE, NUM_REG_1024LINE*1024*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
  SCMULATE_INFOMSG(1, " %ld registers of 2048LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_2048LINE, CACHE_LINE_SIZE*2048, NUM_REG_2048LINE*2048*CACHE_LINE_SIZE, NUM_REG_2048LINE*2048*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);

}


bool
scm::reg_file_module::checkRegisterConfig() {
  if (CALCULATE_REG_SIZE > REG_FILE_SIZE_KB*1000) {
    // This is an error, because it could cause seg fault
    SCMULATE_ERROR(0, "DEFINED REGISTER IS LARGER THAN DEFINED REG_FILE_SIZE_KB");
    SCMULATE_ERROR(0, "REG_FILE_SIZE_KB = %ld", REG_FILE_SIZE_KB*1000l);
    SCMULATE_ERROR(0, "CALCULATE_REG_SIZE = %ld", CALCULATE_REG_SIZE);
    SCMULATE_ERROR(0, "EXCESS = %ld", CALCULATE_REG_SIZE - REG_FILE_SIZE_KB*1000l);
    return 0;
  } else if (CALCULATE_REG_SIZE < REG_FILE_SIZE_KB*1000) {
    // This is just a warning, you are not using the whole register file
    SCMULATE_WARNING(0, "DEFINED REGISTER IS SMALLER THAN DEFINED REG_FILE_SIZE_KB");
    SCMULATE_WARNING(0, "REG_FILE_SIZE_KB = %ld", REG_FILE_SIZE_KB*1000l);
    SCMULATE_WARNING(0, "CALCULATE_REG_SIZE = %ld", CALCULATE_REG_SIZE);
    SCMULATE_WARNING(0, "REMAINING = %ld", (REG_FILE_SIZE_KB*1000l) - CALCULATE_REG_SIZE);
  }

  return 1;
}

void 
scm::reg_file_module::dumpRegister(std::string size, int num) {
  int len_in_bytes = 0;
  if (size == "64B")
    len_in_bytes = 64;
  else if (size == "1L")
    len_in_bytes = 1*CACHE_LINE_SIZE;
  else if (size == "8L")
    len_in_bytes = 8*CACHE_LINE_SIZE;
  else if (size == "16L")
    len_in_bytes = 16*CACHE_LINE_SIZE;
  else if (size == "256L")
    len_in_bytes = 256*CACHE_LINE_SIZE;
  else if (size == "512L")
    len_in_bytes = 512*CACHE_LINE_SIZE;
  else if (size == "1024L")
    len_in_bytes = 1024*CACHE_LINE_SIZE;
  else if (size == "2048L")
    len_in_bytes = 2048*CACHE_LINE_SIZE;
  else
    SCMULATE_ERROR(0, "DECODED REGISTER DOES NOT EXIST!!!")
  std::cout << "reg_" << size << " = 0x" ;
  for (int i = 0; i < len_in_bytes; i++)
    std::cout<< std::setfill('0')<<std::setw(2) << std::hex << static_cast<int>(getRegisterByName(size, num)[i]) << (i%2 != 0? "  ":"");
  std::cout << std::endl;

}

scm::reg_file_module::~reg_file_module() {
  delete reg_file;
}


