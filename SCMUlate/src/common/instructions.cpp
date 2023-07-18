#include "instructions.hpp"
#include "instruction_mem.hpp"

// TODO: update all the functions to use getOpStr and getOp instead of the static op num
//       to allow expansion in the future with multiple number of operands
namespace scm {
    bool 
    decoded_instruction_t::decodeOperands(inst_mem_module * const inst_memory) {
        // TODO: JMPLBL cannot be decoded because the potential of labels that have not been
        // parsed yet. Probably need to find a solution for this. 
        reg_file_module * reg_file_m = inst_memory->getRegisterFileModule();
        for (uint32_t op_num = 1; op_num <= MAX_NUM_OPERANDS; op_num++) {
          std::string & opStr = getOpStr(op_num);
          operand_t & op = getOp(op_num);
          if (opStr.size() != 0 && op.type == operand_t::UNKNOWN) {
            // Check for imm or regisiter
            if (!instructions::isRegister(opStr) && !instructions::isLabel(opStr)) {
              // IMMEDIATE VALUE CASE
              // TODO: Think about the signed option of these operands
              op.type = operand_t::IMMEDIATE_VAL;
              op.value.immediate = std::stoull(opStr);
            } else if (instructions::isRegister(opStr)) {
              // REGISTER REGISTER ADD CASE
              op.type = operand_t::REGISTER;
              op.value.reg = instructions::decodeRegister(opStr);
              op.value.reg.reg_ptr = reg_file_m->getRegisterByName(op.value.reg.reg_size, op.value.reg.reg_number);
              op.value.reg.reg_size_bytes = reg_file_m->getRegisterSizeInBytes(op.value.reg.reg_size);
            } else if (instructions::isLabel(opStr)) {
              op.type = operand_t::LABEL;
              op.value.immediate = inst_memory->getMemoryLabel(opStr);
            } else {
              SCMULATE_ERROR(0, "Operand was not recognized")
            }
            op.read = OP_IO::getOpRDIO(op_num) & this->op_in_out;
            op.write = OP_IO::getOpWRIO(op_num) & this->op_in_out;
          }
        }

        // For codelets
        if (type == EXECUTE_INST) {
          codelet_params newArgs;
          for (uint32_t op_num = 1; op_num <= MAX_NUM_OPERANDS; op_num++) {
            operand_t & op = getOp(op_num);
            if (op.type == operand_t::IMMEDIATE_VAL) {
              newArgs.getParamAs(op_num) = reinterpret_cast<unsigned char *>(op.value.immediate);
            } else if (op.type == operand_t::REGISTER) {
              newArgs.getParamAs(op_num) = op.value.reg.reg_ptr; 
            } else {
              newArgs.getParamAs(op_num) = nullptr;
            }
          }

          cod_exec = scm::codeletFactory::createCodelet(this->getInstruction(), newArgs);
          cod_exec->setMemoryRange(&this->memRanges);
          if (cod_exec == nullptr) 
            return false;
          for (uint32_t op_num = 1; op_num <= MAX_NUM_OPERANDS; op_num++) {
            operand_t & op = getOp(op_num);
            op.read = OP_IO::getOpRDIO(op_num) & cod_exec->getOpIO();
            op.write = OP_IO::getOpWRIO(op_num) & cod_exec->getOpIO();
          }
          this->setOpIO(cod_exec->getOpIO());
        }

        // Calculate directions for registers
        calculateOperandsDirs();
        return true;
    }

    void
    decoded_instruction_t::calculateMemRanges() {
      memRanges.reads.clear();
      memRanges.writes.clear();
      if (this->getType() == instType::MEMORY_INST && this->getOpcode() != LDIMM_INST.opcode) {
        int32_t size_dest = this->getOp1().value.reg.reg_size_bytes;
        unsigned long base_addr = 0;
        unsigned long offset = 0;

        // Check for the memory address
        if (this->getOp2().type == operand_t::IMMEDIATE_VAL) {
          // Load address immediate value
          base_addr = this->getOp2().value.immediate;
        } else if (this->getOp2().type == operand_t::REGISTER) {
          // Load address register value
          decoded_reg_t reg = this->getOp2().value.reg;
          unsigned char *reg_ptr = reg.reg_ptr;
#ifdef ARITH64
          base_addr = *((uint64_t *) reg_ptr);
#else
          int32_t size_reg_bytes = reg.reg_size_bytes;
          int32_t i, j;
          for (i = size_reg_bytes - 1, j = 0; j < 8 || i >= 0; --i, ++j)
          {
            unsigned long temp = reg_ptr[i];
            temp <<= j * 8;
            base_addr += temp;
          }
#endif
        } else {
          SCMULATE_ERROR(0, "Incorrect operand type");
        }
        // Check for offset only on the thisructions with such operand
        if (this->getOpcode() == LDOFF_INST.opcode || this->getOpcode() == STOFF_INST.opcode) {
          if (this->getOp3().type == operand_t::IMMEDIATE_VAL) {
            // Load address immediate value
            offset = this->getOp3().value.immediate;
          } else if (this->getOp3().type == operand_t::REGISTER) {
            // Load address register value
            decoded_reg_t reg = this->getOp3().value.reg;
            unsigned char *reg_ptr = reg.reg_ptr;
#ifdef ARITH64
            offset = *((uint64_t *) reg_ptr);
            SCMULATE_INFOMSG(4, "ARITH64: offset set to 0x%lx", offset);
#else
            int32_t size_reg_bytes = reg.reg_size_bytes;
            int32_t i, j;
            for (i = size_reg_bytes - 1, j = 0; j < 8 || i >= 0; --i, ++j)
            {
              unsigned long temp = reg_ptr[i];
              temp <<= j * 8;
              offset += temp;
            }
#endif
          } else {
            SCMULATE_ERROR(0, "Incorrect operand type");
          }
        }
        if (this->getOpcode() == LDOFF_INST.opcode || this->getOpcode() == LDADR_INST.opcode)
          memRanges.reads.emplace(reinterpret_cast<l2_memory_t>(base_addr + offset), size_dest);
        if (this->getOpcode() == STOFF_INST.opcode || this->getOpcode() == STADR_INST.opcode)
          memRanges.writes.emplace(reinterpret_cast<l2_memory_t>(base_addr + offset), size_dest);
      } else if (this->getType() == instType::EXECUTE_INST && this->cod_exec->isMemoryCodelet()) {
        this->cod_exec->calculateMemRanges();
      }
    }

    bool
    decoded_instruction_t::isOpAnAddress(int op_num) {
      if (this->getType() == instType::MEMORY_INST && this->getOpcode() != LDIMM_INST.opcode) {
        if (op_num != 1) {
          // This is always the source/destination register;
          return true;
        }
      } else if (this->getType() == instType::EXECUTE_INST && this->cod_exec->isMemoryCodelet()) {
        if (this->cod_exec->isOpAnAddress(op_num))
          return true;
      }

      return false;
    }

    void 
      decoded_instruction_t::calculateOperandsDirs () {
        inst_operand_dir.clear();
        for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
          if (this->getOp(i).type == operand_t::REGISTER) {
            // Attempt to insert the register, if it is already there, the insert will return a pair with the iterator to the 
            // already inserted value.
            auto it_insert = inst_operand_dir.insert(std::pair<decoded_reg_t, reg_state>(this->getOp(i).value.reg, reg_state::NONE));
            reg_state &cur_state = it_insert.first->second;

            // We need to set the value of reg_state to read, write or readwrite
            if ( (this->getOp(i).read && this->getOp(i).write ) ||
                 (this->getOp(i).read && cur_state == reg_state::WRITE ) ||
                 (this->getOp(i).write && cur_state == reg_state::READ) ) 
              cur_state = reg_state::READWRITE;
            else if (this->getOp(i).write && cur_state == reg_state::NONE)
              cur_state = reg_state::WRITE;
            else if (this->getOp(i).read && cur_state == reg_state::NONE)
              cur_state = reg_state::READ;
            
            SCMULATE_INFOMSG(5, "In instruction %s Register %s set as %s", this->getFullInstruction().c_str() , this->getOp(i).value.reg.reg_name.c_str(), reg_state_str(cur_state).c_str());
        }
      }
    }

    void 
    decoded_instruction_t::updateCodeletParams() {
      // For codelets only
      if (type == EXECUTE_INST) {
        // TODO: To change the number oof arguments, wee should change this number
        codelet_params& newArgs = cod_exec->getParams();
        if (op1.type == operand_t::REGISTER) {
          newArgs.getParamAs(1) = op1.value.reg.reg_ptr; 
        }
        
        if (op2.type == operand_t::REGISTER) {
          newArgs.getParamAs(2) = op2.value.reg.reg_ptr; 
        }

        if (op3.type == operand_t::REGISTER) {
          newArgs.getParamAs(3) = op3.value.reg.reg_ptr; 
        }
      }
    }
}