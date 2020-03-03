#ifndef __MEMORY_MODULE__
#define __MEMORY_MODULE__

#include "SCMUlate_tools.hpp"
#include "control_store.hpp"
#include "instructions.hpp"
#include "register.hpp"

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
      reg_file_module* reg_file_m;
      volatile bool* aliveSignal;
      unsigned char * memorySpace;

      // This should only be called by this unit
      inline void emptyInstSlot() {
          delete myInstructionSlot;
          // Needs atomic read since it is used for sync purposes 
          // with the SU unit
          #pragma omp atomic write
          myInstructionSlot = NULL;
      }

    public: 
      mem_interface_module() = delete;
      mem_interface_module(unsigned char * const memory, reg_file_module* regFile, bool * aliveSig);

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
      

      int behavior();

      /** \brief logic to execute a memory instruction
       *
       *  We execute memory operations according to the provided instruction
       */
      inline void executeMemoryInstructions();

  
  };
  
}

#endif
