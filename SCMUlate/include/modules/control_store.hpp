#ifndef __CONTROL_STORE__
#define __CONTROL_STORE__

#include "SCMUlate_tools.hpp"
#include "threads_configuration.hpp"
#include "instructions.hpp"
#include "codelet.hpp"
#include <vector>



namespace scm {

  /* An execution slot connects the scheduling of an instruction to its 
   * corresponding executor. The executor will be reading the execution
   * slot waiting for instructions to be available, once there's something
   * it will start its execution, and when it is finished it will remove it
   * At this stage it's assumed the result has been commited either to the memory unit or
   * or the register directly. On the other hand the scheduler will have
   * a list of all the execution slots and chose one according to some 
   * scheduling policy. 
   */
  typedef enum {EMPTY, BUSY, DONE} executorState;
  typedef std::vector< instruction_state_pair* > instructions_queue_t;
  class execution_slot {
    private:
      instructions_queue_t executionQueue;
      instructions_queue_t::iterator head;
      instructions_queue_t::iterator tail;

    public:
      // Constructor 
      execution_slot() {
        for (int i = 0; i < EXECUTION_QUEUE_SIZE; i++) {
          executionQueue.emplace_back(nullptr);
        }
        head = tail = executionQueue.begin();
      };

      bool try_insert(instruction_state_pair *);
      void consume();

      inline bool is_empty() {
        #pragma omp flush acquire
        return *head == nullptr && head == tail;
      }

      void inline getNext (instructions_queue_t::iterator& current) {
        current++; 
        if (current == executionQueue.end())
          current = executionQueue.begin();
      }
      

      inline instruction_state_pair * getHead() const { return *this->head; }
  };


  /* Control store corresponds to the glue logic between the different 
   * modules and the execution of an instruction. The control logic
   * should contain the different execution slots and any additional logic 
   * that allows the connection between the modules that schedule the 
   * instructions and those that compute. Execution slots are created and
   * deleted here
   */
  class control_store_module {
    private:
      std::vector <execution_slot*> execution_slots;

    public: 
     control_store_module() = delete;
     control_store_module(const int numExecUnits);

     inline execution_slot* get_executor(const int exec) const { return this->execution_slots[exec]; }
     inline uint32_t numExecutors() { return execution_slots.size(); }

     ~control_store_module();

  };
  
}

#endif
