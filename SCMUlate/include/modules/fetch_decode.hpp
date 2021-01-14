#ifndef __FETCH_DECODE__
#define __FETCH_DECODE__

/** \brief Fetch decode module 
 *
 * This file contains the definition of the fetch_decode module that is used to 
 * obtain instructions from the instruction memory and pass them to the corresponding
 * execution unit or memory. 
 * 
 * The fetch decode is the equivalent of the Scheduling Unit. However, it uses the ilp unit
 * to make decisions on what is ready and can be scheduled. Furthermore, it uses the instruction_buffer
 * as the scheduling window. The instructions state in the scheduling window determines what 
 * can or cannot be executed. 
 * 
 * Since there are no backward dependencies in the dataflow graph that is formed in the buffer
 * there is no risk of deadlock. The instructions that are executing will eventually liberate any 
 * data dependencies on the instructions that are waiting. 
 * 
 * The algorithm should have this order:
 * The instruction buffer must be cleaned. Cleaning will change the position numbers in the
 *     queue, do not rely on the possition for ILP
 * The instruction buffer is filled with a new instruction.
 * The instruction is passed through the IPL controller to determine the next state
 * The buffer is traversed for instructions that are ready. When an instruction is
 *     ready there is some cleaning that needs to be done. These will be handled by the ILP
 *     controlle
 * The instrucion that is done requires to handle some bookkeping to change the state of 
 *     other instructions. Then the instruction can be set for decomissioning. 
 * The process starts over. 
 * 
 */

#include "SCMUlate_tools.hpp"
#include "instruction_mem.hpp"
#include "control_store.hpp"
#include "register.hpp"
#include "instructions.hpp"
#include "timers_counters.hpp"
#include "ilp_controller.hpp"
#include "instruction_buffer.hpp"
#include "system_config.hpp"
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
      instructions_buffer_module inst_buff_m;
      instruction_state_pair * stallingInstruction;
      //const bool debugger;

      TIMERS_COUNTERS_GUARD(
        std::string su_timer_name;
        timers_counters *time_cnt_m;
      );

    public: 
      fetch_decode_module() = delete;
      fetch_decode_module(inst_mem_module * const inst_mem, control_store_module * const, bool * const aliveSig, ILP_MODES ilp_mode);

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
      inline bool attemptAssignExecuteInstruction(instruction_state_pair * inst);

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
          this->su_timer_name = std::string("SU_") + std::to_string(this->su_number);
          this->time_cnt_m->addTimer(this->su_timer_name, SU_TIMER);
        }
      );
  
  };
  
}

#endif
