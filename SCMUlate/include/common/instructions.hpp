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

#define REGISTER_REGEX "[a-zA-Z0-9]+"
#define INMIDIATE_REGEX "[-]?[0-9]+"

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
  typedef struct inst_def {
    std::string inst_name;
    std::string inst_regex;
    int num_op;
  } inst_def_t;

  // SCM specific insctructions
  const inst_def_t COMMIT_INST = DEF_INST(COMMIT, "[ ]*(COMMIT;).*", 0);                                          /* COMMIT; */
  const inst_def_t LABEL_INST = DEF_INST(LABEL,   "[ ]*([a-zA-Z0-9_]+)[ ]*:.*", 0);                                 /* myLabel: */
  const inst_def_t CODELET_INST = DEF_INST(LABEL, "[ ]*COD[ ]+([a-zA-Z0-9_]+)[ ]+ ([a-zA-Z0-9_, ]*);.*", 0);      /* COD codelet_name arg1, arg2, arg3; */
  
  // CONTROL FLOW INSTRUCTIONS
  /** \brief All the control instructions
   *
   * All the different regular expressions used for Control flow instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static inst_def_t const controlInsts[] = {
    DEF_INST( JMPLBL,  "[ ]*(JMPLBL)[ ]+(" REGISTER_REGEX ");.*", 1),                                                   /* JMPLBL destination;*/
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
      std::string op1;
      std::string op2;
      std::string op3;
   public:
      // Constructors
      decoded_instruction_t (instType type) :
        type(type), instruction(""), op1(""), op2(""), op3("")  {}
      decoded_instruction_t (instType type, std::string inst, std::string op1, std::string op2, std::string op3) :
        type(type), instruction(inst), op1(op1), op2(op2), op3(op3)  {}

      // Getters and setters
      /** \brief get the instruction type
       *  \sa istType
       */
      inline instType getType() { return type; }
      /** \brief get the instruction name
       */
      inline std::string getInstruction() { return instruction; }
      /** \brief get the op1 
       */
      inline std::string getOp1() { return op1; }
      /** \brief get the op2
       */
      inline std::string getOp2() { return op2; }
      /** \brief get the op3
       */
      inline std::string getOp3() { return op3; }


  };

  class instructions {
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
      /** \brief Is the instruction type LABEL
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is LABEL type
       */
      static inline bool isCommit(std::string const inst, decoded_instruction_t ** decInst);
      /** \brief Is the instruction type COMMIT
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is COMMIT type
       */
      static inline bool isLabel(std::string const inst);
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
    instructions::isLabel(std::string const inst) {
      std::regex search_exp(LABEL_INST.inst_regex, std::regex_constants::ECMAScript);
      if (std::regex_match(inst, search_exp))
        return true;
      return false;
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
        *decInst = new decoded_instruction_t(COMMIT);
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


