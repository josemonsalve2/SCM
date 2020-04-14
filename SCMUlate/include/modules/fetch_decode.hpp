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
#include "timers_counters.hpp"
#include "ilp_controller.hpp"
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
      control_store_module * ctrl_st_m; /**< Used to assing operations to the executors */
      bool * aliveSignal; /**< When the machine is done, this flag is set to true finishing all the other units */
      int PC; /**< Program counter, this corresponds to the current instruction being executed */
      uint32_t su_number; /**< This corresponds to the current SU number */
      ilp_controller instructionLevelParallelism;

      TIMERS_COUNTERS_GUARD(
        std::string su_timer_name;
        timers_counters *time_cnt_m;
      );

    public: 
      fetch_decode_module() = delete;
      fetch_decode_module(inst_mem_module * const inst_mem, control_store_module * const, bool * const aliveSig);

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
      inline bool attemptAssignExecuteInstruction(decoded_instruction_t * inst);

      /** \brief get the SU number
       *
       *  We select a CU and we assign a new codelet to it. When it is done, we delete the codelet
       */
      inline uint32_t getSUnum() { return this->su_number; }

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
