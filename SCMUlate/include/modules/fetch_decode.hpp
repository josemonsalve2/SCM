#ifndef __FETCH_DECODE__
#define __FETCH_DECODE__

#include "SCMUlate_tools.hpp"
#include "instruction_mem.hpp"

#define NUM_OF_INST(a) sizeof(a)/sizeof(std::string)

namespace scm {

  // List of possible instructions 
  static std::string const controlInsts[] = {"JMPLBL", "JMPPC", "BREQ", "BGT", "BGET", "BLT", "BLET"};
  static std::string const basicArithInsts[] = {"ADD", "SUB", "SHFL", "SHFR"};
  static std::string const memInsts[] = {"LDADR", "LDOFF", "STADR", "STOFF"};

  enum instType {UNKNOWN, COMMIT, CONTROL_INST, BASIC_ARITH_INST, EXECUTE_INST, MEMORY_INST};
  class fetch_decode_module {
    private:
      inst_mem_module * inst_mem_m;
      bool done;
      int PC; 

      static inline instType findInstType(std::string const);
      static inline bool isControlInst(std::string const);
      static inline bool isBasicArith(std::string const);
      static inline bool isExecution(std::string const);
      static inline bool isMemory(std::string const);
  
    public: 
      fetch_decode_module() = delete;
      fetch_decode_module(inst_mem_module * const inst_mem);

      int behavior();
  
  };
  
}

#endif
