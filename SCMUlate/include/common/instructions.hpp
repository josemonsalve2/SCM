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

#define NUM_OF_INST(a) sizeof(a)/sizeof(std::string)

namespace scm {
  /** \brief All the control instructions
   *
   * All the different regular expressions used for Control flow instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static std::string const controlInsts[] = {
    "JMPLBL[ ]+([a-zA-Z0-9]+);.*", /* JMPLBL destination;*/
    "JMPPC[ ]+([-]*[0-9]+);.*",  /* JMPPC -100;*/
    "BREQ[ ]+([a-zA-Z0-9]+)[ ]*,[ ]*([a-zA-Z0-9]+)[ ]*,([ -]*[0-9]+);.*",   /* BREQ R1, R2, -100; */
    "BGT[ ]+([a-zA-Z0-9]+)[ ]*,[ ]*([a-zA-Z0-9]+)[ ]*,([ -]*[0-9]+);.*",   /* BGT R1, R2, -100; */
    "BGET[ ]+([a-zA-Z0-9]+)[ ]*,[ ]*([a-zA-Z0-9]+)[ ]*,([ -]*[0-9]+);.*",   /* BGET R1, R2, -100; */
    "BLT[ ]+([a-zA-Z0-9]+)[ ]*,[ ]*([a-zA-Z0-9]+)[ ]*,([ -]*[0-9]+);.*",   /* BLT R1, R2, -100; */
    "BLET[ ]+([a-zA-Z0-9]+)[ ]*,[ ]*([a-zA-Z0-9]+)[ ]*,([ -]*[0-9]+);.*"};  /* BLET R1, R2, -100; */
  /** \brief All the basic arithmetic instructions
   *
   * All the different regular expressions used for basic arithmetic instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static std::string const basicArithInsts[] = {
    "ADD[ ]+([a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*;.*", /* ADD R1, R2, R3; R2 and R3 can be literals*/
    "SUB[ ]+([a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*;.*",  /* SUB R1, R2, R3; R2 and R3 can be literals*/
    "SHFL[ ]+([a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*;.*",  /* SHFL R1, R2; R2 can be a literal representing how many positions to shift*/
    "SHFR[ ]+([a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*;.*"};  /* SHFR R1, R2; R2 can be a literal representing how many positions to shift*/
  /** \brief All the memory related instructions
   *
   * All the different regular expressions used for Control flow instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static std::string const memInsts[] = {
    "LDADR[ ]+([a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*;.*", /* LDADR R1, R2; R2 can be a literal or the address in a the register*/
    "LDOFF[ ]+([a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*;.*", /* LDOFF R1, R2, R3; R1 is the base destination register, R2 is the base address, R3 is the offset. R2 and R3 can be literals */
    "STADR[ ]+([a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*;.*", /* LDADR R1, R2; R2 can be a literal or the address in a the register*/
    "STOFF[ ]+([a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*,([ -]*[a-zA-Z0-9]+)[ ]*;.*"}; /* LDOFF R1, R2, R3; R1 is the base destination register, R2 is the base address, R3 is the offset. R2 and R3 can be literals */

  /** \brief Definition of the instruction types 
   *
   *  There are several instructions types. This is important for the decoding of instructions
   *  and for knwoing where to send the instruction for execution 
   */
  enum instType {UNKNOWN, COMMIT, CONTROL_INST, BASIC_ARITH_INST, EXECUTE_INST, MEMORY_INST};

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
      static inline instType findInstType(std::string const inst);
      /** \brief Is the instruction type CONTROL_INST
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is CONTROL type
       *  \sa instType
       */
      static inline bool isControlInst(std::string const inst);
      /** \brief Is the instruction type BASIC_ARITH_INST
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is CONTROL type
       *  \sa instType
       */
      static inline bool isBasicArith(std::string const inst );
      /** \brief Is the instruction type EXECUTE_INST
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is CONTROL type
       *  \sa instType
       */
      static inline bool isExecution(std::string const inst );
      /** \brief Is the instruction type MEMORY_INST
       *  \param inst the corresponding instruction text to identify
       *  \returns true or false if the instruction is CONTROL type
       *  \sa instType
       */
      static inline bool isMemory(std::string const ints );
  };

  instType
    instructions::findInstType(std::string const instruction) {
      if (instruction == "COMMIT") 
        return COMMIT;
      if (isControlInst(instruction))
        return CONTROL_INST;
      if (isBasicArith(instruction))
        return BASIC_ARITH_INST;
      if (isExecution(instruction))
        return EXECUTE_INST;
      if (isMemory(instruction))
        return MEMORY_INST;

      return UNKNOWN;
    }

  bool 
    instructions::isControlInst(std::string const inst) {
      for (size_t i = 0; i < NUM_OF_INST(controlInsts); i++) {
        std::regex search_exp(controlInsts[i]);
        if (std::regex_match(inst, search_exp))
          return true;
      }
      return false;
    }
  bool 
    instructions::isBasicArith(std::string const inst) {
      for (size_t i = 0; i < NUM_OF_INST(basicArithInsts); i++) {
        std::regex search_exp(basicArithInsts[i]);
        if (std::regex_match(inst, search_exp))
          return true;
      }
      return false;
    }
  bool 
    instructions::isExecution(std::string const inst) {
      if (inst.substr(0, 3) == "COD")
        return true;
      return false;
    }
  bool 
    instructions::isMemory(std::string const inst) {
      for (size_t i = 0; i < NUM_OF_INST(memInsts); i++) {
        std::regex search_exp(memInsts[i]);
        if (std::regex_match(inst, search_exp))
          return true;
      }
      return false;
    }

}

#endif


