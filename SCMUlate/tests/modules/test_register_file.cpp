#include "register.hpp"

int main () {
  scm::reg_file_module reg_file_m;

  for (int i = 0; i < 64; i++) reg_file_m.getRegisterByName("64B", 1)[i] = 18;

  reg_file_m.dumpRegister("64B", 1);

  return 0;
}
