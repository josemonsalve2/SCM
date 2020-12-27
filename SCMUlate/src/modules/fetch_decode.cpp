#include "fetch_decode.hpp"
#include <string>
#include <vector>

scm::fetch_decode_module::fetch_decode_module(inst_mem_module *const inst_mem, 
                                              control_store_module *const control_store_m, 
                                              bool *const aliveSig, 
                                              ILP_MODES ilp_mode) : 
                                              inst_mem_m(inst_mem),
                                              ctrl_st_m(control_store_m),
                                              aliveSignal(aliveSig),
                                              PC(0),
                                              su_number(0), 
                                              instructionLevelParallelism(ilp_mode), 
                                              stallingInstruction(nullptr), 
                                              debugger(DEBUGER_MODE)
                                              
{
}

int scm::fetch_decode_module::behavior()
{
  TIMERS_COUNTERS_GUARD(
      this->time_cnt_m->addEvent(this->su_timer_name, SU_START););
  SCMULATE_INFOMSG(1, "Initializing the SU");
// Initialization barrier
#pragma omp barrier
  while (*(this->aliveSignal))
  {
    // FETCHING PC
    scm::decoded_instruction_t *new_inst = this->inst_mem_m->fetch(this->PC);
    SCMULATE_ERROR_IF(0, !new_inst, "Returned instruction is NULL. This should not happen");
    if (!new_inst)
    {
      *(this->aliveSignal) = false;
      continue;
    }
    // Clear out instructions that are decomissioned
    this->inst_buff_m.clean_out_queue();

    // Insert new instruction
    if (this->stallingInstruction == nullptr && this->inst_buff_m.add_instruction(*new_inst)) {
      instructionLevelParallelism.checkMarkInstructionToSched(this->inst_buff_m.get_latest());
      SCMULATE_INFOMSG(5, "Executing PC = %d", this->PC);
      TIMERS_COUNTERS_GUARD(
          this->time_cnt_m->addEvent(this->su_timer_name, FETCH_DECODE_INSTRUCTION););
      // Mark instruction for scheduling
    }

    TIMERS_COUNTERS_GUARD(
        this->time_cnt_m->addEvent(this->su_timer_name, DISPATCH_INSTRUCTION, new_inst->getInstruction()););
    // Iterate over the instruction buffer (window) looking for instructions to execute
    for (auto it = this->inst_buff_m.get_buffer()->begin(); it != this->inst_buff_m.get_buffer()->end(); ++it)
    {
      switch (it->second) {
      case instruction_state::STALL:
        this->stallingInstruction = &(*it);
        instructionLevelParallelism.checkMarkInstructionToSched(&(*it));
        break;
      case instruction_state::WAITING:
        instructionLevelParallelism.checkMarkInstructionToSched(&(*it));
        break;
      case instruction_state::READY:
        switch (it->first->getType()) {
          case COMMIT:
            // Check if there are other instructions in the buffer that need to be finished
            if (this->inst_buff_m.get_buffer()->size() == 1) {
              SCMULATE_INFOMSG(4, "Scheduling and Exec a COMMIT");
              SCMULATE_INFOMSG(1, "Turning off machine alive = false");
              #pragma omp atomic write
              *(this->aliveSignal) = false;
              it->second = instruction_state::EXECUTION_DONE;
            }
            break;
          case CONTROL_INST:
            SCMULATE_INFOMSG(4, "Scheduling a CONTROL_INST %s", it->first->getFullInstruction().c_str());
            TIMERS_COUNTERS_GUARD(
                this->time_cnt_m->addEvent(this->su_timer_name, EXECUTE_CONTROL_INSTRUCTION, it->first->getInstruction()););
            executeControlInstruction(it->first);
            it->second = instruction_state::EXECUTION_DONE;
            break;
          case BASIC_ARITH_INST:
            SCMULATE_INFOMSG(4, "Scheduling a BASIC_ARITH_INST %s", it->first->getFullInstruction().c_str());
            TIMERS_COUNTERS_GUARD(
                this->time_cnt_m->addEvent(this->su_timer_name, EXECUTE_ARITH_INSTRUCTION););
            executeArithmeticInstructions(it->first);
            it->second = instruction_state::EXECUTION_DONE;
            break;
          case EXECUTE_INST:
            SCMULATE_INFOMSG(4, "Scheduling an EXECUTE_INST %s", it->first->getFullInstruction().c_str());
            attemptAssignExecuteInstruction(&(*it));
            break;
          case MEMORY_INST:
            SCMULATE_INFOMSG(4, "Scheduling a MEMORY_INST %s", it->first->getFullInstruction().c_str());
            attemptAssignExecuteInstruction(&(*it));
            break;
          default:
            SCMULATE_ERROR(0, "Instruction not recognized");
            #pragma omp atomic write
            *(this->aliveSignal) = false;
            break;
        }
        break;
      
      case instruction_state::EXECUTION_DONE:
        // check if stalling instruction
        if (this->stallingInstruction != nullptr && &(*it) == this->stallingInstruction)
          this->stallingInstruction = nullptr;
        instructionLevelParallelism.instructionFinished(&(*it));
        it->second = instruction_state::DECOMISION;
        break;
      default:
        break;
      }
      TIMERS_COUNTERS_GUARD(
        this->time_cnt_m->addEvent(this->su_timer_name, SU_IDLE););

      // Check if any instructions have finished
      for (uint32_t i = 0; i < this->ctrl_st_m->numExecutors(); i++) {
        if (this->ctrl_st_m->get_executor(i)->is_done()) {
          this->ctrl_st_m->get_executor(i)->getHead()->second = instruction_state::EXECUTION_DONE;
          SCMULATE_INFOMSG(5, "Instruction %s has finished executing", this->ctrl_st_m->get_executor(i)->getHead()->first->getFullInstruction().c_str());

          this->ctrl_st_m->get_executor(i)->empty_slot();
        }
      }

    }
    // Progress PC, if not stalling
    if (this->stallingInstruction == nullptr) {
      SCMULATE_INFOMSG(5"incrementing PC \n");
      this->PC++;
    }
  }
  SCMULATE_INFOMSG(1, "Shutting down fetch decode unit");
  TIMERS_COUNTERS_GUARD(
      this->time_cnt_m->addEvent(this->su_timer_name, SU_END););
  return 0;
}

void scm::fetch_decode_module::executeControlInstruction(scm::decoded_instruction_t *inst)
{

  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE JMPLBL INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "JMPLBL")
  {
    int newPC = this->inst_mem_m->getMemoryLabel(inst->getOp1Str()) - 1;
    SCMULATE_ERROR_IF(0, newPC == -1, "Incorrect label translation");
    PC = newPC;
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE JMPPC INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "JMPPC")
  {
    int offset = inst->getOp1().value.immediate;
    int target = offset + PC - 1;
    SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0), "Incorrect destination offset");
    PC = target;
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BREQ INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BREQ")
  {
    decoded_reg_t reg1 = inst->getOp1().value.reg;
    decoded_reg_t reg2 = inst->getOp2().value.reg;
    unsigned char *reg1_ptr = reg1.reg_ptr;
    unsigned char *reg2_ptr = reg2.reg_ptr;
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool bitComparison = true;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < reg1.reg_size_bytes; ++i)
    {
      if (reg1_ptr[i] ^ reg2_ptr[i])
      {
        bitComparison = false;
        break;
      }
    }
    if (bitComparison)
    {
      int offset = inst->getOp3().value.immediate;
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0), "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BGT INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BGT")
  {
    decoded_reg_t reg1 = inst->getOp1().value.reg;
    decoded_reg_t reg2 = inst->getOp2().value.reg;
    unsigned char *reg1_ptr = reg1.reg_ptr;
    unsigned char *reg2_ptr = reg2.reg_ptr;
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_gt_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < reg1.reg_size_bytes; ++i)
    {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 > reg2 in that byte, then reg1 > reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] > reg2_ptr[i])
      {
        reg1_gt_reg2 = true;
        break;
      }
    }
    if (reg1_gt_reg2)
    {
      int offset = inst->getOp3().value.immediate;
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0), "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BGET INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BGET")
  {
    decoded_reg_t reg1 = inst->getOp1().value.reg;
    decoded_reg_t reg2 = inst->getOp2().value.reg;
    unsigned char *reg1_ptr = reg1.reg_ptr;
    unsigned char *reg2_ptr = reg2.reg_ptr;
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_get_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    uint32_t size_reg_bytes = reg1.reg_size_bytes;
    for (uint32_t i = 0; i < size_reg_bytes; ++i)
    {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 > reg2 in that byte, then reg1 > reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] > reg2_ptr[i])
      {
        reg1_get_reg2 = true;
        break;
      }
      // If we have not found any byte that is different in both registers from MSB to LSB, and the LSB byte is the same, the the registers are the same
      if (i == size_reg_bytes - 1 && reg1_ptr[i] == reg2_ptr[i])
        reg1_get_reg2 = true;
    }
    if (reg1_get_reg2)
    {
      int offset = inst->getOp3().value.immediate;
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0), "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BLT INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BLT")
  {
    decoded_reg_t reg1 = inst->getOp1().value.reg;
    decoded_reg_t reg2 = inst->getOp2().value.reg;
    unsigned char *reg1_ptr = reg1.reg_ptr;
    unsigned char *reg2_ptr = reg2.reg_ptr;
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_lt_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < reg1.reg_size_bytes; ++i)
    {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 < reg2 in that byte, then reg1 < reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] < reg2_ptr[i])
      {
        reg1_lt_reg2 = true;
        break;
      }
    }
    if (reg1_lt_reg2)
    {
      int offset = inst->getOp3().value.immediate;
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0), "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BLET INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BLET")
  {
    decoded_reg_t reg1 = inst->getOp1().value.reg;
    decoded_reg_t reg2 = inst->getOp2().value.reg;
    unsigned char *reg1_ptr = reg1.reg_ptr;
    unsigned char *reg2_ptr = reg2.reg_ptr;
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_let_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    uint32_t size_reg_bytes = reg1.reg_size_bytes;
    for (uint32_t i = 0; i < size_reg_bytes; ++i)
    {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 < reg2 in that byte, then reg1 < reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] < reg2_ptr[i])
      {
        reg1_let_reg2 = true;
        break;
      }
      // If we have not found any byte that is different in both registers from MSB to LSB, and the LSB byte is the same, the the registers are the same
      if (i == size_reg_bytes - 1 && reg1_ptr[i] == reg2_ptr[i])
        reg1_let_reg2 = true;
    }
    if (reg1_let_reg2)
    {
      int offset = inst->getOp3().value.immediate;
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0), "Incorrect destination offset");
      PC = target;
    }
    return;
  }
}
void scm::fetch_decode_module::executeArithmeticInstructions(scm::decoded_instruction_t *inst)
{
  /////////////////////////////////////////////////////
  ///// ARITHMETIC LOGIC FOR THE ADD INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "ADD")
  {
    decoded_reg_t reg1 = inst->getOp1().value.reg;
    decoded_reg_t reg2 = inst->getOp2().value.reg;
    // Second operand may be register or immediate. We assumme immediate are no longer than a long long
    if (inst->getOp3().type == scm::operand_t::IMMEDIATE_VAL)
    {
      // IMMEDIATE ADDITION CASE
      // TODO: Think about the signed option of these operands
      uint64_t immediate_val = inst->getOp3().value.immediate;

      unsigned char *reg2_ptr = reg2.reg_ptr;

      // Where to store the result
      unsigned char *reg1_ptr = reg1.reg_ptr;
      int32_t size_reg_bytes = reg1.reg_size_bytes;

      // Addition
      uint32_t temp = 0;
      for (int32_t i = size_reg_bytes - 1; i >= 0; --i)
      {
        temp += (immediate_val & 255) + (reg2_ptr[i]);
        reg1_ptr[i] = temp & 255;
        immediate_val >>= 8;
        // Carry on
        temp = temp > 255 ? 1 : 0;
      }
    }
    else
    {
      // REGISTER REGISTER ADD CASE
      decoded_reg_t reg3 = inst->getOp3().value.reg;
      unsigned char *reg2_ptr = reg2.reg_ptr;
      unsigned char *reg3_ptr = reg3.reg_ptr;

      // Where to store the result
      unsigned char *reg1_ptr = reg1.reg_ptr;
      int32_t size_reg_bytes = reg1.reg_size_bytes;

      // Addition
      int temp = 0;
      for (int32_t i = size_reg_bytes - 1; i >= 0; --i)
      {
        temp += (reg3_ptr[i]) + (reg2_ptr[i]);
        reg1_ptr[i] = temp & 255;
        // Carry on
        temp = temp > 255 ? 1 : 0;
      }
    }
    return;
  }

  /////////////////////////////////////////////////////
  ///// ARITHMETIC LOGIC FOR THE SUB INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "SUB")
  {
    decoded_reg_t reg1 = inst->getOp1().value.reg;
    decoded_reg_t reg2 = inst->getOp2().value.reg;

    // Second operand may be register or immediate. We assumme immediate are no longer than a long long
    if (inst->getOp3().type == scm::operand_t::IMMEDIATE_VAL)
    {
      // IMMEDIATE ADDITION CASE
      // TODO: Think about the signed option of these operands
      uint16_t immediate_val = inst->getOp3().value.immediate;

      unsigned char *reg2_ptr = reg2.reg_ptr;

      // Where to store the result
      unsigned char *reg1_ptr = reg1.reg_ptr;
      int32_t size_reg_bytes = reg1.reg_size_bytes;

      // Subtraction
      uint32_t temp = 0;
      for (int32_t i = size_reg_bytes - 1; i >= 0; --i)
      {
        uint32_t cur_byte = immediate_val & 255;
        if (reg2_ptr[i] < cur_byte + temp)
        {
          reg1_ptr[i] = reg2_ptr[i] + 256 - temp - cur_byte;
          temp = 1; // Increase carry
        }
        else
        {
          reg1_ptr[i] = reg2_ptr[i] - temp - cur_byte;
          temp = 0; // Carry has been used
        }
        immediate_val >>= 8;
      }
      SCMULATE_ERROR_IF(0, temp == 1, "Registers must be possitive numbers, addition of numbers resulted in negative number. Carry was 1 at the end of the operation");
    }
    else
    {
      // REGISTER REGISTER ADD CASE
      decoded_reg_t reg3 = inst->getOp3().value.reg;
      unsigned char *reg2_ptr = reg2.reg_ptr;
      unsigned char *reg3_ptr = reg3.reg_ptr;

      // Where to store the result
      unsigned char *reg1_ptr = reg1.reg_ptr;
      int32_t size_reg_bytes = reg1.reg_size_bytes;

      // Subtraction
      uint32_t temp = 0;
      for (int32_t i = size_reg_bytes - 1; i >= 0; --i)
      {
        if (reg2_ptr[i] < reg3_ptr[i] + temp)
        {
          reg1_ptr[i] = reg2_ptr[i] + 256 - temp - reg3_ptr[i];
          temp = 1; // Increase carry
        }
        else
        {
          reg1_ptr[i] = reg2_ptr[i] - temp - reg3_ptr[i];
          temp = 0; // Carry has been used
        }
      }
      SCMULATE_ERROR_IF(0, temp == 1, "Registers must be possitive numbers, addition of numbers resulted in negative number. Carry was 1 at the end of the operation");
    }
    return;
  }

  /////////////////////////////////////////////////////
  ///// ARITHMETIC LOGIC FOR THE SHFL INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "SHFL")
  {
    SCMULATE_ERROR(0, "THE SHFL OPERATION HAS NOT BEEN IMPLEMENTED. KILLING THIS")
#pragma omp atomic write
    *(this->aliveSignal) = false;
  }

  /////////////////////////////////////////////////////
  ///// ARITHMETIC LOGIC FOR THE SHFR INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "SHFR")
  {
    SCMULATE_ERROR(0, "THE SHFR OPERATION HAS NOT BEEN IMPLEMENTED. KILLING THIS")
#pragma omp atomic write
    *(this->aliveSignal) = false;
  }
}

bool scm::fetch_decode_module::attemptAssignExecuteInstruction(scm::instruction_state_pair *inst)
{
  // TODO: Jose this is the point where you can select scheduing policies
  static uint32_t curSched = 0;
  bool sched = false;
  uint32_t attempts = 0;
  // We try scheduling on all the sched units
  while (!sched && attempts++ < this->ctrl_st_m->numExecutors()) {
    curSched++;
    curSched %= this->ctrl_st_m->numExecutors();
    if (this->ctrl_st_m->get_executor(curSched)->is_empty())
    {
      this->ctrl_st_m->get_executor(curSched)->assign(inst);
      sched = true;
    }
  }
  if (!sched)
  {
    SCMULATE_INFOMSG(5, "Could not find a free unit");
  }
  else
  {
    SCMULATE_INFOMSG(5, "Scheduling to CUMEM %d", curSched);
  }
  return sched;
}