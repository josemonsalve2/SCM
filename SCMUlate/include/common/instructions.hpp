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

#define REGISTER_REGEX "R[BbLl0-9_]+"
#define REGISTER_SPLIT_REGEX "R([BbLl0-9]+)_([0-9]+)"
#define INMIDIATE_REGEX "[-]?[0-9]+"
#define LABEL_REGEX "([a-zA-Z0-9_]+)"
#define COMMENT_REGEX "([ ]*//.*)"

#define DEF_INST(name, regExp, numOp) {#name, regExp, numOp}
#define NUM_OF_INST(a) sizeof(a)/sizeof(inst_def_t)

namespace scm {
  /** \brief Instruction definition
   *
   *  This struct contains all the information needed for the definition of a instruction 
   *  from the language perspective
   *
   *  To add an instruction you must create it with DEF_INST() and then add it to the vector for 
   *  the corresponding group
   */
  struct inst_def_t {
    std::string inst_name;
    std::string inst_regex;
    int num_op;
  };

  // SCM specific insctructions
  const inst_def_t COMMIT_INST = DEF_INST(COMMIT, "[ ]*(COMMIT;).*", 0);                                          /* COMMIT; */
  const inst_def_t LABEL_INST = DEF_INST(LABEL,   "[ ]*([a-zA-Z0-9_]+)[ ]*:.*", 0);                                 /* myLabel: */
  const inst_def_t CODELET_INST = DEF_INST(CODELET, "[ ]*COD[ ]+([a-zA-Z0-9_]+)[ ]+([a-zA-Z0-9_, ]*);.*", 0);      /* COD codelet_name arg1, arg2, arg3; */
  
  // CONTROL FLOW INSTRUCTIONS
  /** \brief All the control instructions
   *
   * All the different regular expressions used for Control flow instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static inst_def_t const controlInsts[] = {
    DEF_INST( JMPLBL,  "[ ]*(JMPLBL)[ ]+(" LABEL_REGEX ");.*", 1),                                                   /* JMPLBL destination;*/
    DEF_INST( JMPPC,   "[ ]*(JMPPC)[ ]+(" INMIDIATE_REGEX ");.*", 1),                                                      /* JMPPC -100;*/
    DEF_INST( BREQ,    "[ ]*(BREQ)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX ");.*", 3),            /* BREQ R1, R2, -100; */
    DEF_INST( BGT,     "[ ]*(BGT)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX ");.*", 3),             /* BGT R1, R2, -100; */
    DEF_INST( BGET,    "[ ]*(BGET)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX ");.*", 3),            /* BGET R1, R2, -100; */
    DEF_INST( BLT,     "[ ]*(BLT)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX ");.*", 3),             /* BLT R1, R2, -100; */
    DEF_INST( BLET,    "[ ]*(BLET)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX ");.*", 3)};            /* BLET R1, R2, -100; */

  //BASIC ARITHMETIC INSTRUCTIONS
  /** \brief All the basic arithmetic instructions
   *
   * All the different regular expressions used for basic arithmetic instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static inst_def_t const basicArithInsts[] = {
  DEF_INST( ADD,  "[ ]*(ADD)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3),     /* ADD R1, R2, R3; R2 and R3 can be literals*/
  DEF_INST( SUB,  "[ ]*(SUB)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3),     /* SUB R1, R2, R3; R2 and R3 can be literals*/
  DEF_INST( SHFL, "[ ]*(SHFL)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 2),                            /* SHFL R1, R2; R2 can be a literal representing how many positions to shift*/
  DEF_INST( SHFR, "[ ]*(SHFR)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 2)};                            /* SHFR R1, R2; R2 can be a literal representing how many positions to shift*/

  //MEMORY INSTRUNCTIONS
  /** \brief All the memory related instructions
   *
   * All the different regular expressions used for Control flow instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static inst_def_t const memInsts[] = {
  DEF_INST( LDIMM, "[ ]*(LDIMM)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX ")[ ]*;.*", 2),                          /* LDADR R1, R2; R2 can be a literal or the address in a the register*/
  DEF_INST( LDADR, "[ ]*(LDADR)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 2),                          /* LDADR R1, R2; R2 can be a literal or the address in a the register*/
  DEF_INST( LDOFF, "[ ]*(LDOFF)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3),  /* LDOFF R1, R2, R3; R1 is the base destination register, R2 is the base address, R3 is the offset. R2 and R3 can be literals */
  DEF_INST( STADR, "[ ]*(STADR)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 2),                          /* LDADR R1, R2; R2 can be a literal or the address in a the register*/
  DEF_INST( STOFF, "[ ]*(STOFF)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3)};  /* LDOFF R1, R2, R3; R1 is the base destination register, R2 is the base address, R3 is the offset. R2 and R3 can be literals */

  /** \brief Definition of the instruction types 
   *
   *  There are several instructions types. This is important for the decoding of instructions
   *  and for knwoing where to send the instruction for execution 
   */
  enum instType {UNKNOWN, COMMIT, CONTROL_INST, BASIC_ARITH_INST, EXECUTE_INST, MEMORY_INST};

  /** \brief Decoded register structure
   *  
   *  Contains the register name a register size that is taken out of a encoded register
   *
   */
  struct decoded_reg_t {
    std::string reg_size;
    uint32_t reg_size_bytes;
    uint32_t reg_number;
    unsigned char * reg_ptr;
    
    decoded_reg_t():
      reg_size(""), reg_size_bytes(0), reg_number(0), reg_ptr(nullptr) { };

    decoded_reg_t(std::string sizeStr, uint32_t sizeBytes, uint32_t regNum, unsigned char * ptr):
      reg_size(sizeStr), reg_size_bytes(sizeBytes), reg_number(regNum), reg_ptr(ptr) { };

    decoded_reg_t(const decoded_reg_t & other) {
      reg_size = other.reg_size;
      reg_size_bytes = other.reg_size_bytes;
      reg_number = other.reg_number;
      reg_ptr = other.reg_ptr;
    }

    void operator=(decoded_reg_t const & other) {
      reg_size = other.reg_size;
      reg_size_bytes = other.reg_size_bytes;
      reg_number = other.reg_number;
      reg_ptr = other.reg_ptr;
    }
  };

  /** \brief Decoded operand structure
   *  
   *  An operand can be either a register or an immediate value, it is enconded in a union and it gets used accordingly 
   *  depending on the type.
   *   
   *
   */
  struct operand_t {
    enum {UNKNOWN, REGISTER, IMMEDIATE_VAL} type;
    union value_t {
      uint64_t immediate;
      decoded_reg_t reg;
      value_t (): reg(){}
      ~value_t() {};
    };
    value_t value;

    operand_t() {
      type = UNKNOWN;
    }

    operand_t(operand_t const & other) {
      type = other.type;
      if (type == REGISTER)
        value.reg = other.value.reg;
      else if (type == IMMEDIATE_VAL)
        value.immediate = other.value.immediate;
    }

    operand_t(operand_t && other) {
      type = other.type;
      if (type == REGISTER)
        value.reg = other.value.reg;
      else if (type == IMMEDIATE_VAL)
        value.immediate = other.value.immediate;
    }

    void operator=(operand_t const & other) {
      type = other.type;
      if (type == REGISTER)
        value.reg = other.value.reg;
      else if (type == IMMEDIATE_VAL)
        value.immediate = other.value.immediate;
    }
  };

  class decoded_instruction_t {
    private:
      /** \brief contains the break down of an instruction
       * This class splits out an instruction into its instruction kind and the different operands 
       * innitially we use only 3 address instructions. If it contains 2 or 1 operands the other 
       * are ignored by the fetch_decode
       *
       */
      instType type;
      std::string instruction;
      std::string op1_s;
      std::string op2_s;
      std::string op3_s;
      codelet * cod_exec;
      operand_t op1;
      operand_t op2;
      operand_t op3;

   public:
      // Constructors
      decoded_instruction_t (instType type) :
        type(type), instruction(""), op1_s(""), op2_s(""), op3_s(""), cod_exec(nullptr), op1(), op2(), op3() {}
      decoded_instruction_t (instType type, std::string inst, std::string op1s, std::string op2s, std::string op3s) :
        type(type), instruction(inst), op1_s(op1s), op2_s(op2s), op3_s(op3s), cod_exec(nullptr), op1(), op2(), op3()  {}

      // Getters and setters
      /** \brief get the instruction type
       *  \sa istType
       */
      inline instType getType() { return type; }
      /** \brief get the instruction name
       */
      inline std::string getInstruction() { return instruction; }
      /** \brief get Codelet
       */
      inline codelet * getExecCodelet() { return cod_exec; }
      /** \brief set the instruction name
       */
      inline void setInstruction(std::string newInstName) { instruction = newInstName; }
      /** \brief set the op1 
       */
      inline void setOp1(operand_t newOpVal) { op1 = newOpVal; }
      /** \brief set the op2
       */
      inline void setOp2(operand_t newOpVal) { op2 = newOpVal; }
      /** \brief set the op3
       */
      inline void setOp3(operand_t newOpVal) { op3 = newOpVal; }
      /** \brief set the op1 
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

      void decodeOperands(reg_file_module * const reg_file_m);

      ~decoded_instruction_t() {
        if (type == EXECUTE_INST) {
          // TODO depending on the type this to change the casting
          unsigned char ** params = reinterpret_cast<unsigned char **> (cod_exec->getParams());
          delete[] params;
          delete cod_exec;
        }
      }
  };

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
      static inline bool isLabel(std::string const inst);

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
      if (!dec) dec = new decoded_instruction_t(UNKNOWN);

      SCMULATE_INFOMSG(4, "decoded: {type = %d, opcode = %s, op1 = %s, op2 = %s, op3 = %s}, ", dec->getType(), dec->getInstruction().c_str(), dec->getOp1Str().c_str(), dec->getOp2Str().c_str(), dec->getOp3Str().c_str());
      

      return dec;
    }

  bool 
    instructions::isCommit(std::string const inst, decoded_instruction_t ** decInst) {
      std::regex search_exp(COMMIT_INST.inst_regex, std::regex_constants::ECMAScript);
      std::smatch matches;
      if (std::regex_search(inst.begin(), inst.end(), matches, search_exp)) {
        *decInst = new decoded_instruction_t(COMMIT);
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
    instructions::isLabel(std::string const inst) {
      std::regex search_exp(LABEL_INST.inst_regex, std::regex_constants::ECMAScript);
      if (std::regex_match(inst, search_exp))
        return true;
      return false;
    }

  decoded_reg_t
    instructions::decodeRegister(std::string const op) {
      std::regex search_exp(REGISTER_SPLIT_REGEX, std::regex_constants::ECMAScript);
      std::smatch matches;
      decoded_reg_t res(std::string(""), 0, 0, nullptr);
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
            *decInst = new decoded_instruction_t(CONTROL_INST, matches[1], "" , "", "");
          } else if (controlInsts[i].num_op == 1) {
            *decInst = new decoded_instruction_t(CONTROL_INST, matches[1], matches[2] , "", "");
          } else if (controlInsts[i].num_op == 2) {
            *decInst = new decoded_instruction_t(CONTROL_INST, matches[1], matches[2] , matches[3], "");
          } else if (controlInsts[i].num_op == 3) {
            *decInst = new decoded_instruction_t(CONTROL_INST, matches[1], matches[2] , matches[3], matches[4]);
          } else {
            SCMULATE_ERROR(0, "Unsupported number of operands for CONTROL instruction");
          }
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
            *decInst = new decoded_instruction_t(BASIC_ARITH_INST, matches[1], "" , "", "");
          } else if (basicArithInsts[i].num_op == 1) {
            *decInst = new decoded_instruction_t(BASIC_ARITH_INST, matches[1], matches[2] , "", "");
          } else if (basicArithInsts[i].num_op == 2) {
            *decInst = new decoded_instruction_t(BASIC_ARITH_INST, matches[1], matches[2] , matches[3], "");
          } else if (basicArithInsts[i].num_op == 3) {
            *decInst = new decoded_instruction_t(BASIC_ARITH_INST, matches[1], matches[2] , matches[3], matches[4]);
          } else {
            SCMULATE_ERROR(0, "Unsupported number of operands for BASIC_ARITH_INST instruction");
          }
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
        std::string delimiter = ",";
        size_t pos = 0;
        size_t opNum = 0;
        std::string token;
        *decInst = new decoded_instruction_t(EXECUTE_INST);
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
            *decInst = new decoded_instruction_t(MEMORY_INST, matches[1], "" , "", "");
          } else if (memInsts[i].num_op == 1) {
            *decInst = new decoded_instruction_t(MEMORY_INST, matches[1], matches[2] , "", "");
          } else if (memInsts[i].num_op == 2) {
            *decInst = new decoded_instruction_t(MEMORY_INST, matches[1], matches[2] , matches[3], "");
          } else if (memInsts[i].num_op == 3) {
            *decInst = new decoded_instruction_t(MEMORY_INST, matches[1], matches[2] , matches[3], matches[4]);
          } else {
            SCMULATE_ERROR(0, "Unsupported number of operands for MEMORY_INST instruction");
          }
          return true; 
        }
      }
      decInst = NULL;
      return false;
    }

}

#endif


