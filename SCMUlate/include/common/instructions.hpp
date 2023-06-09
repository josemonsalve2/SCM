#ifndef __INSTRUCTIONS__
#define __INSTRUCTIONS__

/**
 * This file contains all the definitions of the regular expressions that allow to identify 
 * an instruction and its operands. It also defines the enum with the different instruction types
 * that is used by the SCM machine during fetch_decode of the program
 *
 * This file also defines the scm::instructions class that contains static methods that can be used
 * to identify the type of an instruction, as well as to extract the operands of the instruction
 *
 * The purpose of this file is to separate the definiton of the supported instructions
 * from the fetch_decode operation, as well as the instruction meomry operation
 *
 * TODO: This file should be replace by some sort of parser, but for now I have not definied the 
 * language correctly, and I am no front end expert, so this will have to do until then. 
 *
 */

#include <string>
#include <regex>
#include <iostream>
#include "codelet.hpp"
#include "SCMUlate_tools.hpp"
#include "stringHelper.hpp"
#include "register.hpp"
#include "instructions_def.hpp"
#include <unordered_map>

namespace scm {
  // circular reference
  class inst_mem_module;

  enum reg_state { NONE, READ, WRITE, READWRITE };
  #define reg_state_str(state) (\
  state == reg_state::READ ? std::string("reg_state::READ") : \
  state == reg_state::WRITE ? std::string("reg_state::WRITE"): \
  state == reg_state::READWRITE ? std::string("reg_state::READWRITE") : \
  state == reg_state::NONE ? std::string("reg_state::NONE") : \
  std::string("?????"))

  class decoded_instruction_t {
    private:
      /** \brief contains the break down of an instruction
       * This class splits out an instruction into its instruction kind and the different operands 
       * innitially we use only 3 address instructions. If it contains 2 or 1 operands the other 
       * are ignored by the fetch_decode
       *
       */
      instType type;
      opcode_t opcode;
      std::string instruction;
      std::string op1_s;
      std::string op2_s;
      std::string op3_s;
      std::uint_fast16_t op_in_out;
      codelet * cod_exec;
      operand_t op1;
      operand_t op2;
      operand_t op3;
      memranges_pair memRanges;
      std::unordered_map<decoded_reg_t, reg_state> inst_operand_dir;

   public:
      // Constructors
      decoded_instruction_t (instType type, opcode_t opc) :
        type(type), opcode(opc), instruction(""), op1_s(""), op2_s(""), op3_s(""), op_in_out(OP_IO::NO_RD_WR), cod_exec(nullptr), op1(), op2(), op3() {}
      decoded_instruction_t (instType type, opcode_t opcode, std::string inst, std::string op1s = std::string(), std::string op2s = std::string(), std::string op3s = std::string()) :
        type(type), opcode(opcode), instruction(inst), op1_s(op1s), op2_s(op2s), op3_s(op3s), op_in_out(OP_IO::NO_RD_WR), cod_exec(nullptr), op1(), op2(), op3()  {}

      decoded_instruction_t (const decoded_instruction_t &other) :
              type(other.type), opcode(other.opcode), instruction(other.instruction), op1_s(other.op1_s), op2_s(other.op2_s), op3_s(other.op3_s), op_in_out(other.op_in_out),cod_exec(nullptr), op1(other.op1), op2(other.op2), op3(other.op3), memRanges(other.memRanges), inst_operand_dir(other.inst_operand_dir) {
                if (other.cod_exec != nullptr) {
                  codelet_params newParams = other.cod_exec->getParams();
                  this->cod_exec = codeletFactory::createCodelet(getInstruction(), newParams);
                  this->cod_exec->setMemoryRange(&memRanges);
                }
              }
      // Getters and setters
      /** \brief get the instruction type
       *  \sa istType
       */
      inline instType& getType() { return type; }
      /** \brief get the instruction opcode
       */
      inline opcode_t getOpcode() { return opcode; }
      /** \brief get the instruction name
       */
      inline std::string& getInstruction() { return instruction; }

      /** \brief get the instruction string with the actual used registers
       */
      inline std::string getFullInstruction() { 
        std::string fullInstWithRename("");
        fullInstWithRename += instruction + std::string(" ");
        fullInstWithRename += (op1.type == operand_t::REGISTER ? op1.value.reg.reg_name : op1_s) + (op2_s.length() != 0? std::string(", "): std::string(""));
        fullInstWithRename += (op2.type == operand_t::REGISTER ? op2.value.reg.reg_name : op2_s) + (op3_s.length() != 0? std::string(", "): std::string(""));
        fullInstWithRename += (op3.type == operand_t::REGISTER ? op3.value.reg.reg_name : op3_s);
        return fullInstWithRename; 
      }
      /** \brief get Codelet
       */
      inline codelet * getExecCodelet() { return cod_exec; }
      /** \brief set the instruction name
       */
      inline void setInstruction(std::string newInstName) { instruction = newInstName; }
      /** \brief set the op_in_out name
       */
      inline void setOpIO(std::uint_fast16_t opIO) { op_in_out = opIO; }
      /** \brief get the op_in_out name
       */
      inline std::uint_fast16_t getOpIO() { return op_in_out; }
      /** \brief set the op1 
       */
      inline void setOp1(operand_t newOpVal) { op1 = newOpVal; }
      /** \brief set the op2
       */
      inline void setOp2(operand_t newOpVal) { op2 = newOpVal; }
      /** \brief set the op3
       */
      inline void setOp3(operand_t newOpVal) { op3 = newOpVal; }
      /** \brief get operand by number
       * TODO: This is wrong in so many levels. We need to remove the limit
       * on three operands. It should be a little bit more clever than this.
       */
      inline operand_t& getOp(int num) {
        if (num < 1 || num > MAX_NUM_OPERANDS) {
          SCMULATE_ERROR(0, "getOp(num) called with an incorrect operand number");
          num = (num % MAX_NUM_OPERANDS) + 1;
        } 
        if (num == 1)
          return op1;
        else if (num == 2)
          return op2;
        return op3;
      }
      /** \brief get operand string by number
       * TODO: This is wrong in so many levels. We need to remove the limit
       * on three operands. It should be a little bit more clever than this.
       */
      inline std::string& getOpStr(int num) {
        if (num < 1 || num > MAX_NUM_OPERANDS) {
          SCMULATE_ERROR(0, "getOp(num) called with an incorrect operand number");
          num = (num % MAX_NUM_OPERANDS) + 1;
        } 
        if (num == 1)
          return op1_s;
        else if (num == 2)
          return op2_s;
        return op3_s;
      }
      /** \brief get the op1 
       */
      inline operand_t& getOp1() { return op1; }
      /** \brief get the op2
       */
      inline operand_t& getOp2() { return op2; }
      /** \brief get the op3
       */
      inline operand_t& getOp3() { return op3; }
      /** \brief get the op1_s
       */
      inline std::string getOp1Str() { return op1_s; }
      /** \brief get the op2_s
       */
      inline std::string getOp2Str() { return op2_s; }
      /** \brief get the op3_s
       */
      inline std::string getOp3Str() { return op3_s; }
      /** \brief set the op1_s 
       */
      inline void setOp1Str(std::string str) { op1_s = str; }
      /** \brief set the op2_s
       */
      inline void setOp2Str(std::string str) { op2_s = str; }
      /** \brief set the op3_s
       */
      inline void setOp3Str(std::string str) { op3_s = str; }

      bool decodeOperands(inst_mem_module * const reg_file_m);

      /** \brief Update codelet register references to connect with renaming
       * 
       * When a Codelet is created its params are defined from the interpreted arguments
       * However, when we rename the operands, we neet ot then re-synchronize the params for the Codelet.
       * 
       */
      
      void updateCodeletParams();

       /** \brief Get the set of memory addresses that concern this instruction or codelet
       * 
       * To avoid memory hazzards, it is important to respect the order of memory opoerations occur for a 
       * single memory location. This method allows to return the memory values needed for the executin
       * of this instruction. Allowing an in-order memory access.
       * 
       */
      memranges_pair* getMemoryRange() { return &this->memRanges; }
      void calculateMemRanges();

      /** \brief for each register, store if read, write or readwrite
       * 
       * This depends on the operands used. For example if you have ADD R1, R1, R1. Then the 
       * register R1 is both read and write. It does not only depends on the operand position
       * 
       */
      void calculateOperandsDirs();

      std::unordered_map<decoded_reg_t, reg_state>* getOperandsDirs() {return &inst_operand_dir; }

      /** \brief Tells if a register represents an address value of a memory instruction or codelet
       * 
       * When an operand represents an address and this one has not been calculated, it is not 
       * possible to avoid hazzards on memory operantions. It is then necessary to know if an address
       * has not been yet calculated. This method allows to tell if an operand should onsider
       * this restriction or not.
       */
      bool isOpAnAddress(int op_num); 

      bool inline isMemoryInstruction() { return this->type == instType::MEMORY_INST ||  (this->type == instType::EXECUTE_INST && this->cod_exec->isMemoryCodelet()); }

      ~decoded_instruction_t() {
        if (type == EXECUTE_INST) {
          // TODO depending on the type this to change the casting
          delete cod_exec;
        }
      }
  };

  // helps the fetch decode determine if the instruction is ready or not for execution
  // The ilp_controller helps the fetch-decode unit to change this state.
  enum instruction_state {
    WAITING, READY, EXECUTING, EXECUTION_DONE, DECOMMISSION, STALL
  };
  typedef std::pair<decoded_instruction_t*, instruction_state> instruction_state_pair; 

  class instructions {
    private:

    public:
      /** \brief Helper instructions class for the identification of the instructions
       *
       *  This class provides the necessary methods to identify what type of instructions 
       *  there a particular string is. It also contains other helper functions 
       */
      instructions() {};

      /** \brief Find the instruction type
       *  \param inst the corresponding instruction text to identify
       *  \returns the instType with the corresponding type
       *  \sa instType
       */
      static inline decoded_instruction_t* findInstType(std::string const inst);

      /** \brief Is the instruction type COMMENT
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is COMMENT type
       */
      static inline bool isComment(std::string const inst);

      /** \brief Is the instruction type LABEL
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is LABEL type
       */
      static inline bool isLabelInst(std::string const inst);

      /** \brief Is the operand a LABEL
       *  \param op the corresponding operand text to identify
       *  \returns true or false if the operand is LABEL type
       */
      static inline bool isLabel(std::string const op);

      /** \brief Is the instruction type COMMIT
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is COMMIT type
       */
      static inline bool isCommit(std::string const inst, decoded_instruction_t ** decInst);

      /** \brief Obtain name and size of register
       *  \param op the operand that contains the enconded register
       *  \returns a decoded_reg_t that contains name and value separately
       *  \sa decoded_reg_t
       */
      static inline decoded_reg_t decodeRegister(std::string const op);

      /** \brief Is the operand a register type or inmediate value
       *  \param op the operand that we want to check
       *  \returns true if the operand encodes a register, false otherwise 
       */
      static inline bool isRegister(std::string const op);

      /** \brief Is the instruction type CONTROL_INST
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is CONTROL type
       *  \sa instType
       */
      static inline bool isControlInst(std::string const inst, decoded_instruction_t ** decInst);

      /** \brief Is the instruction type BASIC_ARITH_INST
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is CONTROL type
       *  \sa instType
       */
      static inline bool isBasicArith(std::string const inst, decoded_instruction_t ** decInst);

      /** \brief Is the instruction type EXECUTE_INST
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is CONTROL type
       *  \sa instType
       */
      static inline bool isExecution(std::string const inst, decoded_instruction_t ** decInst);
      
      /** \brief Is the instruction type MEMORY_INST
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is CONTROL type
       *  \sa instType
       */
      static inline bool isMemory(std::string const ints, decoded_instruction_t ** decInst);

      /** \brief extract the label from the instruction name
       *  \param inst the corresponding instruction text to extract the label from
       *  \returns the label in a string
       *  \se isLabel
       */
      static inline std::string getLabel(std::string const inst);

  };

  decoded_instruction_t *
    instructions::findInstType(std::string const instruction) {
      decoded_instruction_t* dec = NULL;
      isCommit(instruction, &dec);
      if (!dec) isControlInst(instruction, &dec);
      if (!dec) isBasicArith(instruction, &dec);
      if (!dec) isExecution(instruction, &dec);
      if (!dec) isMemory(instruction, &dec);
      if (!dec) dec = new decoded_instruction_t(UNKNOWN, 0xFF);
      return dec;
    }

  bool 
    instructions::isCommit(std::string const inst, decoded_instruction_t ** decInst) {
      std::regex search_exp(COMMIT_INST.inst_regex, std::regex_constants::ECMAScript);
      std::smatch matches;
      if (std::regex_search(inst.begin(), inst.end(), matches, search_exp)) {
        *decInst = new decoded_instruction_t(COMMIT, COMMIT_INST.opcode, matches[1]);
        return true;
      }
      decInst = NULL;
      return false;
    }

  bool 
    instructions::isRegister(std::string const op) {
      std::regex search_exp(REGISTER_REGEX, std::regex_constants::ECMAScript);
      if (std::regex_match(op, search_exp))
        return true;
      return false;
    }

  bool 
    instructions::isComment(std::string const inst) {
      std::regex search_exp(COMMENT_REGEX, std::regex_constants::ECMAScript);
      if (std::regex_match(inst, search_exp))
        return true;
      return false;
    }

  bool 
    instructions::isLabel(std::string const op) {
      std::regex search_exp(LABEL_REGEX, std::regex_constants::ECMAScript);
      if (std::regex_match(op, search_exp))
        return true;
      return false;
    }

  bool 
    instructions::isLabelInst(std::string const inst) {
      std::regex search_exp(LABEL_INST.inst_regex, std::regex_constants::ECMAScript);
      if (std::regex_match(inst, search_exp))
        return true;
      return false;
    }

  decoded_reg_t
    instructions::decodeRegister(std::string const op) {
      std::regex search_exp(REGISTER_SPLIT_REGEX, std::regex_constants::ECMAScript);
      std::smatch matches;
      decoded_reg_t res(op, std::string(""), 0, 0, nullptr);
      if (std::regex_search(op.begin(), op.end(), matches, search_exp)) {
         res.reg_size = matches[1];
         res.reg_number = std::stoi(matches[2]);
      }
      return res;
    }

  std::string
    instructions::getLabel(std::string const inst) {
      std::regex search_exp(LABEL_INST.inst_regex, std::regex_constants::ECMAScript);
      std::smatch matches;
      if (std::regex_search(inst.begin(), inst.end(), matches, search_exp)) {
        return matches[1];
      }
      return "";
    }

  bool 
    instructions::isControlInst(std::string const inst, decoded_instruction_t ** decInst) {
      for (size_t i = 0; i < NUM_OF_INST(controlInsts); i++) {
        std::regex search_exp(controlInsts[i].inst_regex, std::regex_constants::ECMAScript);
        std::smatch matches;
        if (std::regex_search(inst.begin(), inst.end(), matches, search_exp)) {
          if (controlInsts[i].num_op == 0) {
            *decInst = new decoded_instruction_t(CONTROL_INST, controlInsts[i].opcode, matches[1], "" , "", "");
          } else if (controlInsts[i].num_op == 1) {
            *decInst = new decoded_instruction_t(CONTROL_INST, controlInsts[i].opcode, matches[1], matches[2] , "", "");
          } else if (controlInsts[i].num_op == 2) {
            *decInst = new decoded_instruction_t(CONTROL_INST, controlInsts[i].opcode, matches[1], matches[2] , matches[3], "");
          } else if (controlInsts[i].num_op == 3) {
            *decInst = new decoded_instruction_t(CONTROL_INST, controlInsts[i].opcode, matches[1], matches[2] , matches[3], matches[4]);
          } else {
            SCMULATE_ERROR(0, "Unsupported number of operands for CONTROL instruction");
          }
          (*decInst)->setOpIO(controlInsts[i].op_in_out);
          return true; 
        }
      }
      decInst = NULL;
      return false;
    }

  bool 
    instructions::isBasicArith(std::string const inst, decoded_instruction_t ** decInst) {
      for (size_t i = 0; i < NUM_OF_INST(basicArithInsts); i++) {
        std::regex search_exp(basicArithInsts[i].inst_regex, std::regex_constants::ECMAScript);
        std::smatch matches;
        if (std::regex_search(inst.begin(), inst.end(), matches, search_exp)) {
          if (basicArithInsts[i].num_op == 0) {
            *decInst = new decoded_instruction_t(BASIC_ARITH_INST, basicArithInsts[i].opcode, matches[1], "" , "", "");
          } else if (basicArithInsts[i].num_op == 1) {
            *decInst = new decoded_instruction_t(BASIC_ARITH_INST, basicArithInsts[i].opcode, matches[1], matches[2] , "", "");
          } else if (basicArithInsts[i].num_op == 2) {
            *decInst = new decoded_instruction_t(BASIC_ARITH_INST, basicArithInsts[i].opcode, matches[1], matches[2] , matches[3], "");
          } else if (basicArithInsts[i].num_op == 3) {
            *decInst = new decoded_instruction_t(BASIC_ARITH_INST, basicArithInsts[i].opcode, matches[1], matches[2] , matches[3], matches[4]);
          } else {
            SCMULATE_ERROR(0, "Unsupported number of operands for BASIC_ARITH_INST instruction");
          }
          (*decInst)->setOpIO(basicArithInsts[i].op_in_out);
          return true; 
        }
      }
      decInst = NULL;
      return false;
    }

  bool 
    instructions::isExecution(std::string const inst, decoded_instruction_t ** decInst) {
      std::regex search_exp(CODELET_INST.inst_regex, std::regex_constants::ECMAScript);
      std::smatch matches;
      if (std::regex_search(inst.begin(), inst.end(), matches, search_exp)) {
        std::string operands = matches[2];
        std::string delimiter(",");
        size_t pos = 0;
        size_t opNum = 0;
        std::string token;
        *decInst = new decoded_instruction_t(EXECUTE_INST, CODELET_INST.opcode);
        (*decInst)->setInstruction(matches[1]);
        while (operands.length() != 0) {
          pos = operands.find(delimiter);
          if (pos == std::string::npos) pos = operands.length();
          token = operands.substr(0, pos);
          token = trim(token);
          if (opNum == 0) {
            (*decInst)->setOp1Str(token);
          } else if (opNum == 1) {
            (*decInst)->setOp2Str(token);
          } else if (opNum == 2) {
            (*decInst)->setOp3Str(token);
          } else {
            SCMULATE_ERROR(0, "Unsupported number of operands for EXECUTE_INST instruction");
            break;
          }
          operands.erase(0, pos + delimiter.length());
          opNum++;
        }
        return true; 
      }
      decInst = NULL;
      return false;
    }
    
  bool 
    instructions::isMemory(std::string const inst, decoded_instruction_t ** decInst) {
      for (size_t i = 0; i < NUM_OF_INST(memInsts); i++) {
        std::regex search_exp(memInsts[i].inst_regex, std::regex_constants::ECMAScript);
        std::smatch matches;
        if (std::regex_search(inst.begin(), inst.end(), matches, search_exp)) {
          if (memInsts[i].num_op == 0) {
            *decInst = new decoded_instruction_t(MEMORY_INST, memInsts[i].opcode, matches[1], "" , "", "");
          } else if (memInsts[i].num_op == 1) {
            *decInst = new decoded_instruction_t(MEMORY_INST, memInsts[i].opcode, matches[1], matches[2] , "", "");
          } else if (memInsts[i].num_op == 2) {
            *decInst = new decoded_instruction_t(MEMORY_INST, memInsts[i].opcode, matches[1], matches[2] , matches[3], "");
          } else if (memInsts[i].num_op == 3) {
            *decInst = new decoded_instruction_t(MEMORY_INST, memInsts[i].opcode, matches[1], matches[2] , matches[3], matches[4]);
          } else {
            SCMULATE_ERROR(0, "Unsupported number of operands for MEMORY_INST instruction");
          }
          (*decInst)->setOpIO(memInsts[i].op_in_out);
          return true; 
        }
      }
      decInst = NULL;
      return false;
    }

}

#endif


