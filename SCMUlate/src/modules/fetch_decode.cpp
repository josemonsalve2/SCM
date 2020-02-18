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
  // Initialization barrier
  #pragma omp barrier
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

void
scm::fetch_decode_module::executeControlInstruction(scm::decoded_instruction_t * inst) {

  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE JMPLBL INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "JMPLBL") {
    int newPC = this->inst_mem_m->getMemoryLabel(inst->getOp1()) - 1;
    SCMULATE_ERROR_IF(0, newPC == -2,   "Incorrect label translation");
    PC = newPC;
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE JMPPC INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "JMPPC") {
    int offset = std::stoi(inst->getOp1());
    int target = offset + PC - 1;
    SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
    PC = target;
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BREQ INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BREQ" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool bitComparison = true;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size); ++i) {
      if (reg1_ptr[i] ^ reg2_ptr[i]) {
        bitComparison = false;
        break;
      }
    }
    if (bitComparison) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BGT INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BGT" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_gt_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size); ++i) {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 > reg2 in that byte, then reg1 > reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] > reg2_ptr[i]) {
        reg1_gt_reg2 = true;
        break;
      }
    }
    if (reg1_gt_reg2) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BGET INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BGET" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_get_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    uint32_t size_reg_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);
    for (uint32_t i = 0; i < size_reg_bytes; ++i) {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 > reg2 in that byte, then reg1 > reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] > reg2_ptr[i]) {
        reg1_get_reg2 = true;
        break;
      }
      // If we have not found any byte that is different in both registers from MSB to LSB, and the LSB byte is the same, the the registers are the same
      if (i == size_reg_bytes-1  && reg1_ptr[i] == reg2_ptr[i])
        reg1_get_reg2 = true;
    }
    if (reg1_get_reg2) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BLT INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BLT" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_lt_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size); ++i) {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 < reg2 in that byte, then reg1 < reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] < reg2_ptr[i]) {
        reg1_lt_reg2 = true;
        break;
      }
    }
    if (reg1_lt_reg2) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BLET INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BLET" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_let_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    uint32_t size_reg_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);
    for (uint32_t i = 0; i < size_reg_bytes; ++i) {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 < reg2 in that byte, then reg1 < reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] < reg2_ptr[i]) {
        reg1_let_reg2 = true;
        break;
      }
      // If we have not found any byte that is different in both registers from MSB to LSB, and the LSB byte is the same, the the registers are the same
      if (i == size_reg_bytes-1 && reg1_ptr[i] == reg2_ptr[i])
        reg1_let_reg2 = true;
    }
    if (reg1_let_reg2) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }

}
