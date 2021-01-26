#include "cod_print.hpp"
#include <iomanip>

IMPLEMENT_CODELET(print,
  // Obtaining the parameters
  unsigned char *reg = this->getParams().getParamValueAs<unsigned char *>(1);
  uint64_t len_in_bytes = this->getParams().getParamValueAs<uint64_t>(2);
  std::cout << " = 0x";
  for (uint64_t i = 0; i < len_in_bytes; i++)
    std::cout<< std::setfill('0')<<std::setw(2) << std::hex << static_cast<unsigned short>(reg[i] & 255) << (i%2 != 0? " ":"");
  std::cout << std::endl;
);