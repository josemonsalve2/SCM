#include "memory_interface.hpp"

scm::mem_interface_module::mem_interface_module(l2_memory_t const memory, cu_executor_module * execMod):
  myInstructionSlot(nullptr),
  executorModule(execMod),
  memorySpace(memory)
  { }

int
scm::mem_interface_module::behavior() {
  if (!this->isInstSlotEmpty()) {
    if (myInstructionSlot->getType() == MEMORY_INST)
      executeMemoryInstructions();
    else 
      executeMemoryCodelet();
    emptyInstSlot();
  }
  return 0;
}

void
scm::mem_interface_module::executeMemoryCodelet() {
    scm::codelet * curCodelet = myInstructionSlot->getExecCodelet();
    curCodelet->setExecutor(this->executorModule);
    curCodelet->implementation();
}
void

scm::mem_interface_module::executeMemoryInstructions() {
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE LDIMM INSTRUCTION
  ///// Operand 1 is where to load the instructions
  ///// Operand 2 the inmediate value to be used
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getOpcode() == LDIMM_INST.opcode) {
    // Obtain destination register
    decoded_reg_t reg1 = myInstructionSlot->getOp1().value.reg;
    unsigned char * reg1_ptr = reg1.reg_ptr;
    int32_t size_reg1_bytes = reg1.reg_size_bytes;

    int32_t i, j;

    // Obtain base address and perform copy
    uint64_t immediate_value = myInstructionSlot->getOp2().value.immediate;
    
    // Perform actual memory assignment
#ifdef ARITH64
    *((uint64_t *) reg1_ptr) = immediate_value;
    SCMULATE_INFOMSG(4, "immediate value %d loaded", immediate_value);
#else
    for (i = size_reg1_bytes-1, j = 0; i >= 0; --i, ++j) {
      if (j < 8) {
        unsigned char temp = immediate_value & 255;
        reg1_ptr[i] = temp;
        immediate_value >>= 8;
      } else {
        // Zero out the rest of the register
        reg1_ptr[i] = 0;
      }
    }
#endif
    return;
  }
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE LDADDR INSTRUCTION
  ///// Operand 1 is where to load the instructions
  ///// Operand 2 is the memory address either a register or immediate value. Only consider 64 bits
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getOpcode() == LDADR_INST.opcode) {
    // Obtain destination register
    decoded_reg_t reg1 = myInstructionSlot->getOp1().value.reg;
    unsigned char * reg1_ptr = reg1.reg_ptr;
    int32_t size_reg1_bytes = reg1.reg_size_bytes;

    int32_t i, j;

    // Obtain base address and perform copy
    unsigned long base_addr = 0;
    if (myInstructionSlot->getOp2().type == operand_t::IMMEDIATE_VAL) {
      // Load address immediate value
      base_addr = myInstructionSlot->getOp2().value.immediate;
    } else if (myInstructionSlot->getOp2().type == operand_t::REGISTER) {
      // Load address register value
      decoded_reg_t reg2 = myInstructionSlot->getOp2().value.reg;
      unsigned char * reg2_ptr = reg2.reg_ptr;
#ifdef ARITH64
      base_addr = *((uint64_t *) reg2_ptr);
#else
      int32_t size_reg2_bytes = reg2.reg_size_bytes;
      for (i = size_reg2_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg2_ptr[i];
        temp <<= j*8;
        base_addr += temp;
      } 
#endif
    } else {
      SCMULATE_ERROR(0, "Incorrect operand type");
    }
    // Perform actual memory copy
    SCMULATE_INFOMSG(4, "Loading 0x%lx from addr 0x%lx (based on root of memory)", *((uint64_t *) this->getAddress(base_addr)),base_addr);
    std::memcpy(reg1_ptr, this->getAddress(base_addr), size_reg1_bytes);
    return;
  }
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE LDOFF INSTRUCTION
  ///// Operand 1 is where to load the instructions
  ///// Operand 2 is the memory address either a register or inmediate value. Only consider 64 bits
  ///// Operand 3 is the offset from the address memoery. Either register or inmediate value. Only consider 64 bits
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getOpcode() == LDOFF_INST.opcode) {
    // Destination register
    decoded_reg_t reg1 = myInstructionSlot->getOp1().value.reg;
    unsigned char * reg1_ptr = reg1.reg_ptr;
    int32_t size_reg1_bytes = reg1.reg_size_bytes;
    unsigned long base_addr = 0;
    unsigned long offset = 0;
    int32_t i, j;


    // Obtaining the base_addr
    if (myInstructionSlot->getOp2().type == operand_t::IMMEDIATE_VAL) {
      // Load address inmediate value
      base_addr = myInstructionSlot->getOp2().value.immediate;
    } else if (myInstructionSlot->getOp2().type == operand_t::REGISTER) {
      // Load address register value
      decoded_reg_t reg2 = myInstructionSlot->getOp2().value.reg;
      unsigned char * reg2_ptr = reg2.reg_ptr;
#ifdef ARITH64
      base_addr = *((uint64_t *) reg2_ptr);
#else
      int32_t size_reg2_bytes = reg2.reg_size_bytes;
      for (i = size_reg2_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg2_ptr[i];
        temp <<= j*8;
        base_addr += temp;
      }
#endif
    } else {
      SCMULATE_ERROR(0, "Incorrect operand type");
    }

    // Obtaining the offset
    if (myInstructionSlot->getOp3().type == operand_t::IMMEDIATE_VAL) {
      // Load address inmediate value
      offset = myInstructionSlot->getOp3().value.immediate;
    } else if (myInstructionSlot->getOp3().type == operand_t::REGISTER) {
      // Load address register value
      decoded_reg_t reg3 = myInstructionSlot->getOp3().value.reg;
      unsigned char * reg3_ptr = reg3.reg_ptr;
#ifdef ARITH64
      base_addr += *((uint64_t *) reg3_ptr);
#else
      int32_t size_reg3_bytes = reg3.reg_size_bytes;
      for (i = size_reg3_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg3_ptr[i];
        temp <<= j*8;
        base_addr += temp;
      }
#endif
    } else {
      SCMULATE_ERROR(0, "Incorrect operand type");
    }
    SCMULATE_INFOMSG(4, "LDOFF loading from %p", this->getAddress(base_addr+offset));
    std::memcpy(reg1_ptr, this->getAddress(base_addr+offset), size_reg1_bytes);
    return;
  }
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE STADDR INSTRUCTION
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getOpcode() == STADR_INST.opcode) {
    // Obtain destination register
    decoded_reg_t reg1 = myInstructionSlot->getOp1().value.reg;
    unsigned char * reg1_ptr = reg1.reg_ptr;
    int32_t size_reg1_bytes = reg1.reg_size_bytes;

    int32_t i, j;

    // Obtain base address and perform copy
    unsigned long base_addr = 0;
    if (myInstructionSlot->getOp2().type == operand_t::IMMEDIATE_VAL) {
      // Load address inmediate value
      base_addr = myInstructionSlot->getOp2().value.immediate;
    } else if (myInstructionSlot->getOp2().type == operand_t::REGISTER) {
      // Load address register value
      decoded_reg_t reg2 = myInstructionSlot->getOp2().value.reg;
      unsigned char * reg2_ptr = reg2.reg_ptr;
#ifdef ARITH64
      base_addr = *((uint64_t *) reg2_ptr);
      SCMULATE_INFOMSG(4, "ARITH64 storing value 0x%lx to addr 0x%lx (based on root of memory)", *((uint64_t *)reg1_ptr), base_addr);
#else
      int32_t size_reg2_bytes = reg2.reg_size_bytes;
      for (i = size_reg2_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg2_ptr[i];
        temp <<= j*8;
        base_addr += temp;
      }
#endif
    } else {
      SCMULATE_ERROR(0, "Incorrect operand type");
    }
    // Perform actual memory copy
    std::memcpy(this->getAddress(base_addr), reg1_ptr, size_reg1_bytes);
    return;
  }
  /////////////////////////////////////////////////////
  ///// LOGIC FOR THE STOFF INSTRUCTION
  /////////////////////////////////////////////////////
  if (this->myInstructionSlot->getOpcode() == STOFF_INST.opcode) {
    // Destination register
    decoded_reg_t reg1 = myInstructionSlot->getOp1().value.reg;
    unsigned char * reg1_ptr = reg1.reg_ptr;
    int32_t size_reg1_bytes = reg1.reg_size_bytes;
    unsigned long base_addr = 0;
    unsigned long offset = 0;
    int32_t i, j;


    // Obtaining the base_addr
    if (myInstructionSlot->getOp2().type == operand_t::IMMEDIATE_VAL) {
      // Load address inmediate value
      base_addr = myInstructionSlot->getOp2().value.immediate;
    } else if (myInstructionSlot->getOp2().type == operand_t::REGISTER) {
      // Load address register value
      decoded_reg_t reg2 = myInstructionSlot->getOp2().value.reg;
      unsigned char * reg2_ptr = reg2.reg_ptr;
#ifdef ARITH64
      base_addr = *((uint64_t *) reg2_ptr);
#else
      int32_t size_reg2_bytes = reg2.reg_size_bytes;
      for (i = size_reg2_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg2_ptr[i];
        temp <<= j*8;
        base_addr += temp;
      }
#endif
    } else {
      SCMULATE_ERROR(0, "Incorrect operand type");
    }

    // Obtaining the offset
    if (myInstructionSlot->getOp3().type == operand_t::IMMEDIATE_VAL) {
      // Load address inmediate value
      offset = myInstructionSlot->getOp3().value.immediate;
    } else if (myInstructionSlot->getOp3().type == operand_t::REGISTER) {
      // Load address register value
      decoded_reg_t reg3 = myInstructionSlot->getOp3().value.reg;
      unsigned char * reg3_ptr = reg3.reg_ptr;
#ifdef ARITH64
      base_addr += *((uint64_t *) reg3_ptr);
#else
      int32_t size_reg3_bytes = reg3.reg_size_bytes;
      for (i = size_reg3_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
        unsigned long temp = reg3_ptr[i];
        temp <<= j*8;
        base_addr += temp;
      }
#endif
    } else {
      SCMULATE_ERROR(0, "Incorrect operand type");
    }

    std::memcpy(this->getAddress(base_addr+offset), reg1_ptr, size_reg1_bytes);
    return;
  }
}
