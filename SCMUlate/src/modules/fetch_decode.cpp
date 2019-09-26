#include "fetch_decode.hpp"

scm::fetch_decode_module::fetch_decode_module(inst_mem_module * const inst_mem, bool * const alive_sig) {
  this->inst_mem_m = inst_mem;
  this->PC = 0;
  this->alive = alive_sig;
}

int
scm::fetch_decode_module::behavior() {
  SCMULATE_INFOMSG(1, "I am an SU");
  while (*(this->alive_sig)) { }
    return 0;
}
