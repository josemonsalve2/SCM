#include "fetch_decode.hpp"
#include <string> 
#include <vector> 

scm::fetch_decode_module::fetch_decode_module(inst_mem_module * const inst_mem, control_store_module * const control_store_m, reg_file_module * const reg_file_m, bool * const aliveSig):
  inst_mem_m(inst_mem),
  reg_file_m(reg_file_m),
  ctrl_st_m(control_store_m),
  aliveSignal(aliveSig),
  PC(0) { }

int
scm::fetch_decode_module::behavior() {
  SCMULATE_INFOMSG(1, "I am an SU");
    while (*(this->aliveSignal)) {
      std::string current_instruction = this->inst_mem_m->fetch(this->PC);
      SCMULATE_INFOMSG(3, "I received instruction: %s", current_instruction.c_str());
      scm::decoded_instruction_t* cur_inst = scm::instructions::findInstType(current_instruction);
      SCMULATE_ERROR_IF(0, !cur_inst, "Returned instruction is NULL. This should not happen");
      // Depending on the instruction do something 
      switch(cur_inst->getType()) {
        case COMMIT:
          SCMULATE_INFOMSG(4, "I've identified a COMMIT");
          SCMULATE_INFOMSG(1, "Turning off machine alive = false");
          #pragma omp atomic write
          *(this->aliveSignal) = false;
          break;
        case CONTROL_INST:
          SCMULATE_INFOMSG(4, "I've identified a CONTROL_INST");
          executeControlInstruction(cur_inst);
          break;
        case BASIC_ARITH_INST:
          SCMULATE_INFOMSG(4, "I've identified a BASIC_ARITH_INST");
          break;
        case EXECUTE_INST:
          SCMULATE_INFOMSG(4, "I've identified a EXECUTE_INST");
          break;
        case MEMORY_INST:
          SCMULATE_INFOMSG(4, "I've identified a MEMORY_INST");
          break;
        default:
          SCMULATE_ERROR(0, "Instruction not recognized [%s]", current_instruction.c_str());
          #pragma omp atomic write
          *(this->aliveSignal) = false;
          break;
      }
      this->PC++;

      delete cur_inst;
    } 
    SCMULATE_INFOMSG(1, "Shutting down fetch decode unit");
    return 0;
}

char*
scm::fetch_decode_module::decodeRegisterName(std::string const reg) {
  
  return NULL;
}

void
scm::fetch_decode_module::executeControlInstruction(scm::decoded_instruction_t * inst) {


}
