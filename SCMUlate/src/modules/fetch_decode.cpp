#include "fetch_decode.hpp"
#include <string> 
#include <vector> 

scm::fetch_decode_module::fetch_decode_module(inst_mem_module * const inst_mem, control_store_module * const control_store_m):
  inst_mem_m(inst_mem),
  ctrl_st_m(control_store_m),
  done(false),
  PC(0) { }

int
scm::fetch_decode_module::behavior() {
  SCMULATE_INFOMSG(1, "I am an SU");
    while (!done) {
      std::string current_instruction = this->inst_mem_m->fetch(this->PC);
      SCMULATE_INFOMSG(3, "I received instruction: %s", current_instruction.c_str());
      instType cur_type = findInstType(current_instruction);
      switch(cur_type) {
        case COMMIT:
          SCMULATE_INFOMSG(4, "I've identified a COMMIT");
          this->done = true;
          break;
        case CONTROL_INST:
          SCMULATE_INFOMSG(4, "I've identified a CONTROL_INST");
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
          break;
      }
      this->PC++;
    } 
    return 0;
}


scm::instType
scm::fetch_decode_module::findInstType(std::string const instruction) {

  if (instruction == "COMMIT") 
    return COMMIT;
  if (isControlInst(instruction))
    return CONTROL_INST;
  if (isBasicArith(instruction))
    return BASIC_ARITH_INST;
  if (isExecution(instruction))
    return EXECUTE_INST;
  if (isMemory(instruction))
    return MEMORY_INST;

  return UNKNOWN;
}


bool 
scm::fetch_decode_module::isControlInst(std::string const inst) {
  for (size_t i = 0; i < NUM_OF_INST(controlInsts); i++)
    if (inst.substr(0, controlInsts[i].length()) == controlInsts[i])
      return true;
  return false;
}
bool 
scm::fetch_decode_module::isBasicArith(std::string const inst) {
  for (size_t i = 0; i < NUM_OF_INST(basicArithInsts); i++)
    if (inst.substr(0, basicArithInsts[i].length()) == basicArithInsts[i])
      return true;
  return false;
}
bool 
scm::fetch_decode_module::isExecution(std::string const inst) {
  if (inst.substr(0, 3) == "COD")
    return true;
  return false;
}
bool 
scm::fetch_decode_module::isMemory(std::string const inst) {
  for (size_t i = 0; i < NUM_OF_INST(memInsts); i++)
    if (inst.substr(0, memInsts[i].length()) == memInsts[i])
      return true;
  return false;
}
