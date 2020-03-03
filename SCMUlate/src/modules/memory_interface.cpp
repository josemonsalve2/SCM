#include "memory_interface.hpp"

scm::mem_interface_module::mem_interface_module(unsigned char * const memory, reg_file_module* regFile, bool * aliveSig):
  reg_file_m(regFile),
  aliveSignal(aliveSig),
  memorySpace(memory) {
}

int
scm::mem_interface_module::behavior() {
  SCMULATE_INFOMSG(1, "Starting MEM MODULE behavior");
  // Initialization barrier
  #pragma omp barrier
  while (*(this->aliveSignal)) {
      if (!this->isInstSlotEmpty()) {
        executeMemoryInstructions();
        emptyInstSlot();
      }
  }
  SCMULATE_INFOMSG(1, "Shutting down MEMORY MODULE");
  return 0;
}
 
void
scm::mem_interface_module::executeMemoryInstructions() {
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE LDADDR INSTRUCTION
  ///// Operand 1 is where to load the instructions
  ///// Operand 2 the inmediate value to be used
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getInstruction() == "LDIMM") {
    // Obtain destination register
    decoded_reg_t reg1 = instructions::decodeRegister(myInstructionSlot->getOp1());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    int32_t size_reg1_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);

    int32_t i, j;

    // Obtain base address and perform copy
    unsigned long immediate_value = std::stoul(myInstructionSlot->getOp2());
    
    // Perform actual memory copy
    for (i = size_reg1_bytes, j = 0; i >= 0; --i, ++j) {
      if (j < 8) {
        unsigned char temp = immediate_value & 255;
        reg1_ptr[i] = temp;
        temp>>=8;
      } else {
        // Zero out the rest of the register
        reg1_ptr[i] = 0;
      }
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE LDADDR INSTRUCTION
  ///// Operand 1 is where to load the instructions
  ///// Operand 2 is the memory address either a register or inmediate value. Only cosider 64 bits
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getInstruction() == "LDADDR") {
    // Obtain destination register
    decoded_reg_t reg1 = instructions::decodeRegister(myInstructionSlot->getOp1());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    int32_t size_reg1_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);

    int32_t i, j;

    // Obtain base address and perform copy
    unsigned long base_addr = 0;
    if (!instructions::isRegister(myInstructionSlot->getOp2())) {
      // Load address inmediate value
      base_addr = std::stoul(myInstructionSlot->getOp2());
    } else {
      // Load address register value
      decoded_reg_t reg2 = instructions::decodeRegister(myInstructionSlot->getOp2());
      unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
      int32_t size_reg2_bytes = this->reg_file_m->getRegisterSizeInBytes(reg2.reg_size);
      for (i = size_reg2_bytes, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg2_ptr[i];
        temp <<= j;
        base_addr += temp;
      }
      
    }
    // Perform actual memory copy
    for (i = 0; i < size_reg1_bytes; i++) {
      reg1_ptr[i] = this->memorySpace[base_addr + i];
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE LDOFF INSTRUCTION
  ///// Operand 1 is where to load the instructions
  ///// Operand 2 is the memory address either a register or inmediate value. Only consider 64 bits
  ///// Operand 3 is the offset from the address memoery. Either register or inmediate value. Only consider 64 bits
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getInstruction() == "LDOFF") {
    // Destination register
    decoded_reg_t reg1 = instructions::decodeRegister(myInstructionSlot->getOp1());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    int32_t size_reg1_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);
    unsigned long base_addr = 0;
    unsigned long offset = 0;
    int32_t i, j;


    // Obtaining the base_addr
    if (!instructions::isRegister(myInstructionSlot->getOp2())) {
      // Load address inmediate value
      base_addr = std::stoul(myInstructionSlot->getOp2());
    } else {
      // Load address register value
      decoded_reg_t reg2 = instructions::decodeRegister(myInstructionSlot->getOp2());
      unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
      int32_t size_reg2_bytes = this->reg_file_m->getRegisterSizeInBytes(reg2.reg_size);
      for (i = size_reg2_bytes, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg2_ptr[i];
        temp <<= j;
        base_addr += temp;
      }
    }

    // Obtaining the offset
    if (!instructions::isRegister(myInstructionSlot->getOp3())) {
      // Load address inmediate value
      offset = std::stoul(myInstructionSlot->getOp3());
    } else {
      // Load address register value
      decoded_reg_t reg3 = instructions::decodeRegister(myInstructionSlot->getOp3());
      unsigned char * reg3_ptr = this->reg_file_m->getRegisterByName(reg3.reg_size, reg3.reg_number);
      int32_t size_reg3_bytes = this->reg_file_m->getRegisterSizeInBytes(reg3.reg_size);
      for (i = size_reg3_bytes, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg3_ptr[i];
        temp <<= j;
        base_addr += temp;
      }
    }

    for (i = 0; i < size_reg1_bytes; i++) {
      reg1_ptr[i] = this->memorySpace[base_addr + offset + i];
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE STADDR INSTRUCTION
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getInstruction() == "STADR") {
    // Obtain destination register
    decoded_reg_t reg1 = instructions::decodeRegister(myInstructionSlot->getOp1());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    int32_t size_reg1_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);

    int32_t i, j;

    // Obtain base address and perform copy
    unsigned long base_addr = 0;
    if (!instructions::isRegister(myInstructionSlot->getOp2())) {
      // Load address inmediate value
      base_addr = std::stoul(myInstructionSlot->getOp2());
    } else {
      // Load address register value
      decoded_reg_t reg2 = instructions::decodeRegister(myInstructionSlot->getOp2());
      unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
      int32_t size_reg2_bytes = this->reg_file_m->getRegisterSizeInBytes(reg2.reg_size);
      for (i = size_reg2_bytes, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg2_ptr[i];
        temp <<= j;
        base_addr += temp;
      }
      
    }
    // Perform actual memory copy
    for (i = 0; i < size_reg1_bytes; i++) {
      this->memorySpace[base_addr + i] = reg1_ptr[i];
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE STOFF INSTRUCTION
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getInstruction() == "STOFF") {
    // Destination register
    decoded_reg_t reg1 = instructions::decodeRegister(myInstructionSlot->getOp1());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    int32_t size_reg1_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);
    unsigned long base_addr = 0;
    unsigned long offset = 0;
    int32_t i, j;


    // Obtaining the base_addr
    if (!instructions::isRegister(myInstructionSlot->getOp2())) {
      // Load address inmediate value
      base_addr = std::stoul(myInstructionSlot->getOp2());
    } else {
      // Load address register value
      decoded_reg_t reg2 = instructions::decodeRegister(myInstructionSlot->getOp2());
      unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
      int32_t size_reg2_bytes = this->reg_file_m->getRegisterSizeInBytes(reg2.reg_size);
      for (i = size_reg2_bytes, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg2_ptr[i];
        temp <<= j;
        base_addr += temp;
      }
    }

    // Obtaining the offset
    if (!instructions::isRegister(myInstructionSlot->getOp3())) {
      // Load address inmediate value
      offset = std::stoul(myInstructionSlot->getOp3());
    } else {
      // Load address register value
      decoded_reg_t reg3 = instructions::decodeRegister(myInstructionSlot->getOp3());
      unsigned char * reg3_ptr = this->reg_file_m->getRegisterByName(reg3.reg_size, reg3.reg_number);
      int32_t size_reg3_bytes = this->reg_file_m->getRegisterSizeInBytes(reg3.reg_size);
      for (i = size_reg3_bytes, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg3_ptr[i];
        temp <<= j;
        base_addr += temp;
      }
    }

    for (i = 0; i < size_reg1_bytes; i++) {
      this->memorySpace[base_addr + offset + i] = reg1_ptr[i];
    }
    return;
  }
}
