#ifndef __MEMORY_MODULE__
#define __MEMORY_MODULE__

#include "SCMUlate_tools.hpp"
#include "control_store.hpp"
#include "instructions.hpp"
#include "register.hpp"
#include "timers_counters.hpp"


namespace scm {

  /** brief: Executors 
   *
   *  The mem_interface performs the execution of memory operations,
   *  it has a single slot for memory instructions that are interpreted
   *  by this unit
   * */
  class mem_interface_module{
    private:
      decoded_instruction_t* myInstructionSlot;
      volatile bool* aliveSignal;
      unsigned char * memorySpace;
      uint32_t memId;

      TIMERS_COUNTERS_GUARD(
        std::string mem_timer_name;
        timers_counters* timer_cnt_m;
      )

      // This should only be called by this unit
      inline void emptyInstSlot() {
          // Needs atomic read since it is used for sync purposes 
          // with the SU unit
          #pragma omp atomic write
          myInstructionSlot = nullptr;
      }

    public: 
      mem_interface_module() = delete;
      mem_interface_module(unsigned char * const memory, bool * aliveSig);

      inline bool isInstSlotEmpty() {
          decoded_instruction_t* temp;
          // Needs atomic read since it is used for sync purposes 
          // with the SU unit
          #pragma omp atomic read
          temp = myInstructionSlot;

          return temp == NULL;
      }

      inline void assignInstSlot(decoded_instruction_t *newInst) {
          // Needs atomic read since it is used for sync purposes 
          // with the SU unit
          #pragma omp atomic write
          myInstructionSlot = newInst;
      }
      
      TIMERS_COUNTERS_GUARD(
        inline void setTimerCnt(timers_counters * tmc) { 
          this->timer_cnt_m = tmc;
          this->mem_timer_name = "MEM_" + std::to_string(this->memId);
          this->timer_cnt_m->addTimer(this->mem_timer_name, MEM_TIMER);
        }
      )

      int behavior();

      /** \brief logic to execute a memory instruction
       *
       *  We execute memory operations according to the provided instruction
       */
      inline void executeMemoryInstructions();

  
  };
  
}

#endif
