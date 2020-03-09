#include "cod_print.hpp"
#include <iomanip>

IMPLEMENT_CODELET(print,
  // Obtaining the parameters
  unsigned char ** args = static_cast<unsigned char **>(this->getParams());
  unsigned char *reg = args[0];
  int len_in_bytes = 8;
  std::cout << " = 0x";
  for (int i = 0; i < len_in_bytes; i++)
    std::cout<< std::setfill('0')<<std::setw(2) << std::hex << static_cast<unsigned short>(reg[i] & 255) << (i%2 != 0? " ":"");
  std::cout << std::endl;
);