#ifndef __FETCH_DECODE__
#define __FETCH_DECODE__

/** \brief Fetch decode module 
 *
 * This file contains the definition of the fetch_decode module that is used to 
 * obtain instructions from the instruction memory and pass them to the corresponding
 * execution unit or memory. If the operation can be easily performed by this unit
 * then it is executed (e.g. a summation). Otherwise, if it is more complex then it 
 * will have to be passed to an execution unit
 */

#include "SCMUlate_tools.hpp"
#include "instruction_mem.hpp"
#include "control_store.hpp"
#include "register.hpp"
#include "instructions.hpp"
#include <string>


namespace scm {

  class fetch_decode_module {
    private:

      /** \brief Fetch Decode of instructions
       *
       * Uses the instruction memory to fetch different 
       * instructions according to the program counter. 
       * Assign instructions to the execution slots. 
       *  
       */
      inst_mem_module * inst_mem_m; /**< From where the instructions are read*/
      reg_file_module * reg_file_m; /**< Used to identify and read the registers to be used by the instructions */
      control_store_module * ctrl_st_m; /**< Used to assing operations to the executors */
      bool * aliveSignal; /**< When the machine is done, this flag is set to true finishing all the other units */
      int PC; /**< Program counter, this corresponds to the current instruction being executed */

      /** \brief Decode a register name and return the actual memory location 
       * This is also used to send the registers to the executors, as well as for executing simple operations
       * \param reg is a text containing the register to decode
       * \return a pointer to the corresponding register, if it exists. Otherwise returns NULL
       */
      static inline char* decodeRegisterName(std::string const reg);

  
    public: 
      fetch_decode_module() = delete;
      fetch_decode_module(inst_mem_module * const inst_mem, control_store_module * const, reg_file_module * const, bool * const aliveSig);

      /** \brief logic to execute an instruction
       * 
       *  Function to execute control instructions that affect the program counter
       */
      static inline void executeControlInstruction(std::string instruction);

      /** Actual logic of this unit
       * 
       * Implements the actual behavior logic of the fetch decode unit
       */
      int behavior();
  
  };
  
}

#endif
