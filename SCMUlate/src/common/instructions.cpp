#include "instructions.hpp"

namespace scm {
    bool 
    decoded_instruction_t::decodeOperands(reg_file_module * const reg_file_m) {
        // TODO: JMPLBL cannot be decoded because the potential of labels that have not been
        // parsed yet. Probably need to find a solution for this. 
        if (this->opcode != JMPLBL_INST.opcode) { 
          if (op1_s.size() != 0 && op1.type == operand_t::UNKNOWN) {
            // Check for imm or regisiter
            if (!instructions::isRegister(op1_s)) {
              // IMMEDIATE VALUE CASE
              // TODO: Think about the signed option of these operands
              op1.type = operand_t::IMMEDIATE_VAL;
              op1.value.immediate = std::stoull(op1_s);
            } else {
              // REGISTER REGISTER ADD CASE
              op1.type = operand_t::REGISTER;
              op1.value.reg = instructions::decodeRegister(op1_s);
              op1.value.reg.reg_ptr = reg_file_m->getRegisterByName(op1.value.reg.reg_size, op1.value.reg.reg_number);
              op1.value.reg.reg_size_bytes = reg_file_m->getRegisterSizeInBytes(op1.value.reg.reg_size);
            }
            op1.read = OP_IO::OP1_RD & this->op_in_out;
            op1.write = OP_IO::OP1_WR & this->op_in_out;
          }
          if (op2_s.size() != 0  && op2.type == operand_t::UNKNOWN) {
            // Check for imm or regisiter
            if (!instructions::isRegister(op2_s)) {
              // IMMEDIATE VALUE CASE
              // TODO: Think about the signed option of these operands
              op2.type = operand_t::IMMEDIATE_VAL;
              op2.value.immediate = std::stoull(op2_s);
            } else {
              // REGISTER REGISTER ADD CASE
              op2.type = operand_t::REGISTER;
              op2.value.reg = instructions::decodeRegister(op2_s);
              op2.value.reg.reg_ptr = reg_file_m->getRegisterByName(op2.value.reg.reg_size, op2.value.reg.reg_number);
              op2.value.reg.reg_size_bytes = reg_file_m->getRegisterSizeInBytes(op2.value.reg.reg_size);
            }
            op2.read = OP_IO::OP2_RD & this->op_in_out;
            op2.write = OP_IO::OP2_WR & this->op_in_out;
          }
          if (op3_s.size() != 0  && op3.type == operand_t::UNKNOWN) {
            // Check for imm or regisiter
            if (!instructions::isRegister(op3_s)) {
              // IMMEDIATE VALUE CASE
              // TODO: Think about the signed option of these operands
              op3.type = operand_t::IMMEDIATE_VAL;
              op3.value.immediate = std::stoull(op3_s);
            } else {
              // REGISTER REGISTER ADD CASE
              op3.type = operand_t::REGISTER;
              op3.value.reg = instructions::decodeRegister(op3_s);
              op3.value.reg.reg_ptr = reg_file_m->getRegisterByName(op3.value.reg.reg_size, op3.value.reg.reg_number);
              op3.value.reg.reg_size_bytes = reg_file_m->getRegisterSizeInBytes(op3.value.reg.reg_size);
            }
            op3.read = OP_IO::OP3_RD & this->op_in_out;
            op3.write = OP_IO::OP3_WR & this->op_in_out;
          }
        }

        // For codelets
        if (type == EXECUTE_INST) {
          // TODO: To change the number oof arguments, wee should change this number
          codelet_params newArgs;
          if (op1.type == operand_t::IMMEDIATE_VAL) {
            newArgs.getParamAs(0) = reinterpret_cast<unsigned char *>(op1.value.immediate);
          } else if (op1.type == operand_t::REGISTER) {
            newArgs.getParamAs(0) = op1.value.reg.reg_ptr; 
          } else {
            newArgs.getParamAs(0) = nullptr;
          }

          if (op2.type == operand_t::IMMEDIATE_VAL) {
            newArgs.getParamAs(1) = reinterpret_cast<unsigned char *>(op2.value.immediate);
          } else if (op2.type == operand_t::REGISTER) {
            newArgs.getParamAs(1) = op2.value.reg.reg_ptr; 
          } else {
            newArgs.getParamAs(1) = nullptr;
          }

          if (op3.type == operand_t::IMMEDIATE_VAL) {
            newArgs.getParamAs(2) = reinterpret_cast<unsigned char *>(op3.value.immediate);
          } else if (op3.type == operand_t::REGISTER) {
            newArgs.getParamAs(2) = op3.value.reg.reg_ptr; 
          } else {
            newArgs.getParamAs(2) = nullptr;
          }
          cod_exec = scm::codeletFactory::createCodelet(this->getInstruction(), newArgs);
          cod_exec->setMemoryRange(&this->memRanges);
          if (cod_exec == nullptr) 
            return false;
          op1.read = OP_IO::OP1_RD & cod_exec->getOpIO();
          op1.write = OP_IO::OP1_WR & cod_exec->getOpIO();
          op2.read = OP_IO::OP2_RD & cod_exec->getOpIO();
          op2.write = OP_IO::OP2_WR & cod_exec->getOpIO();
          op3.read = OP_IO::OP3_RD & cod_exec->getOpIO();
          op3.write = OP_IO::OP3_WR & cod_exec->getOpIO();
          this->setOpIO(cod_exec->getOpIO());
      }
      return true;
    }

    void
    decoded_instruction_t::calculateMemRanges() {
      memRanges.clear();
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
          int32_t size_reg_bytes = reg.reg_size_bytes;
          int32_t i, j;
          for (i = size_reg_bytes - 1, j = 0; j < 8 || i >= 0; --i, ++j)
          {
            unsigned long temp = reg_ptr[i];
            temp <<= j * 8;
            base_addr += temp;
          }
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
            int32_t size_reg_bytes = reg.reg_size_bytes;
            int32_t i, j;
            for (i = size_reg_bytes - 1, j = 0; j < 8 || i >= 0; --i, ++j)
            {
              unsigned long temp = reg_ptr[i];
              temp <<= j * 8;
              offset += temp;
            }
          } else {
            SCMULATE_ERROR(0, "Incorrect operand type");
          }
        }
        memRanges.emplace(reinterpret_cast<l2_memory_t>(base_addr + offset), size_dest);
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
    decoded_instruction_t::updateCodeletParams() {
      // For codelets only
      if (type == EXECUTE_INST) {
        // TODO: To change the number oof arguments, wee should change this number
        codelet_params& newArgs = cod_exec->getParams();
        if (op1.type == operand_t::REGISTER) {
          newArgs.getParamAs(0) = op1.value.reg.reg_ptr; 
        }
        
        if (op2.type == operand_t::REGISTER) {
          newArgs.getParamAs(1) = op2.value.reg.reg_ptr; 
        }

        if (op3.type == operand_t::REGISTER) {
          newArgs.getParamAs(2) = op3.value.reg.reg_ptr; 
        }
      }
    }
}