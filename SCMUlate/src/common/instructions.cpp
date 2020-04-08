#include "instructions.hpp"

namespace scm {
    void 
    decoded_instruction_t::decodeOperands(reg_file_module * const reg_file_m) {
        // TODO: JMPLBL cannot be decoded because the potential of labels that have not been
        // parsed yet. Probably need to find a solution for this. 
        if (this->instruction != "JMPLBL") { 
          if (op1_s != "" && op1.type == operand_t::UNKNOWN) {
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
          if (op2_s != "" && op2.type == operand_t::UNKNOWN) {
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
          if (op3_s != "" && op3.type == operand_t::UNKNOWN) {
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
          unsigned char ** newArgs = new unsigned char*[3];
          if (op1.type == operand_t::IMMEDIATE_VAL) {
            newArgs[0] = reinterpret_cast<unsigned char *>(op1.value.immediate);
          } else if (op1.type == operand_t::REGISTER) {
            newArgs[0] = op1.value.reg.reg_ptr; 
          } else {
            newArgs[0] = nullptr;
          }

          if (op2.type == operand_t::IMMEDIATE_VAL) {
            newArgs[1] = reinterpret_cast<unsigned char *>(op2.value.immediate);
          } else if (op2.type == operand_t::REGISTER) {
            newArgs[1] = op2.value.reg.reg_ptr; 
          } else {
            newArgs[1] = nullptr;
          }

          if (op3.type == operand_t::IMMEDIATE_VAL) {
            newArgs[2] = reinterpret_cast<unsigned char *>(op3.value.immediate);
          } else if (op3.type == operand_t::REGISTER) {
            newArgs[2] = op3.value.reg.reg_ptr; 
          } else {
            newArgs[2] = nullptr;
          }
          cod_exec = scm::codeletFactory::createCodelet(this->getInstruction(), newArgs);
          op1.read = OP_IO::OP1_RD & cod_exec->getOpIO();
          op1.write = OP_IO::OP1_WR & cod_exec->getOpIO();
          op2.read = OP_IO::OP2_RD & cod_exec->getOpIO();
          op2.write = OP_IO::OP2_WR & cod_exec->getOpIO();
          op3.read = OP_IO::OP3_RD & cod_exec->getOpIO();
          op3.write = OP_IO::OP3_WR & cod_exec->getOpIO();
      }
    }
}