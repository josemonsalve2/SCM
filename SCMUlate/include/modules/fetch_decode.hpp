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
#include "memory_interface.hpp"
#include "timers_counters.hpp"
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
      mem_interface_module * mem_interface_m; /**< Used to assing memory instructions for execution */
      bool * aliveSignal; /**< When the machine is done, this flag is set to true finishing all the other units */
      int PC; /**< Program counter, this corresponds to the current instruction being executed */
      uint32_t su_number; /**< This corresponds to the current SU number */

      TIMERS_COUNTERS_GUARD(
        std::string su_timer_name;
        timers_counters *time_cnt_m;
      );

    public: 
      fetch_decode_module() = delete;
      fetch_decode_module(inst_mem_module * const inst_mem, reg_file_module * const, control_store_module * const, mem_interface_module * const mem_int, bool * const aliveSig);

      /** \brief logic to execute an instruction
       * 
       *  Function to execute control instructions that affect the program counter
       */
      inline void executeControlInstruction(decoded_instruction_t * inst);

      /** \brief logic to execute an arithmetic instruction
       *
       *  We will execute arithmetic instructions in the SU when they are simple enough. Other arithmetic
       *  instructions must be implemented as codelets 
       */
      inline void executeArithmeticInstructions(decoded_instruction_t * inst);


      /** \brief logic to execute an arithmetic instruction
       *
       *  We select a CU and we assign a new codelet to it. When it is done, we delete the codelet
       */
      inline void assignExecuteInstruction(decoded_instruction_t * inst);

      /** Actual logic of this unit
       * 
       * Implements the actual behavior logic of the fetch decode unit
       */
      int behavior();

      TIMERS_COUNTERS_GUARD(
        void setTimerCounter(timers_counters * newTC) { 
          this->time_cnt_m = newTC; 
          this->su_timer_name = "SU_" + std::to_string(this->su_number);
          this->time_cnt_m->addTimer(this->su_timer_name, SU_TIMER);
        }
      );
  
  };
  
}

#endif
