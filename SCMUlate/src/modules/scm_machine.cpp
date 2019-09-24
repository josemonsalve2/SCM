#include "scm_machine.hpp"

scm::scm_machine::scm_machine(filename): alive(false) {
  strcpy(this->filename, filename);

  // === INITIALIZATION ====
  // We check the register configuration is valid
  if(!reg_file_m.checkRegisterConfig()) {
    SCMUlate(0, "Error when checking the register");
    init_correct = false;
    return;
  }

  init_correct = true;
}

