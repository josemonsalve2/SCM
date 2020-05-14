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
      l2_memory_t memorySpace;

      // This should only be called by this unit
      inline void emptyInstSlot() {
          myInstructionSlot = nullptr;
      }

    public: 
      mem_interface_module() = delete;
      mem_interface_module(unsigned char * const memory);
      inline l2_memory_t getAddress(uint64_t address) { return this->memorySpace + address; }

      inline bool isInstSlotEmpty() {
          return myInstructionSlot == NULL;
      }

      inline void assignInstSlot(decoded_instruction_t *newInst) {
          myInstructionSlot = newInst;
      }

      int behavior();

      /** \brief logic to execute a memory instruction
       *
       *  We execute memory operations according to the provided instruction
       */
      inline void executeMemoryInstructions();

  
  };
  
}

#endif
