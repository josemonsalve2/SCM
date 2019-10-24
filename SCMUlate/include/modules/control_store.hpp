#ifndef __CONTROL_STORE__
#define __CONTROL_STORE__

#include "SCMUlate_tools.hpp"
#include "threads_configuration.hpp"
#include "codelet.hpp"
#include <vector>



namespace scm {

  /* An execution slot connects the scheduling of an instruction to its 
   * corresponding executor. The executor will be reading the execution
   * slot waiting for something to be available, once there's something
   * it will remove start its execution, and when it is finished it will
   * cleare it for the scheduler to assign more work. At this stage it's 
   * assumed the result has been commited either to the memory unit or
   * or the register directly. On the other hand the scheduler will have
   * a list of all the execution slots and chose one according to some 
   * scheduling policy. 
   */
  class execution_slot {
    private:
      bool empty;
      codelet *executionCodelet;

    public:
      // Constructor 
      execution_slot(): empty(true), executionCodelet(NULL) {}; 

      void assign(codelet *);
      void empty_slot();
      inline bool is_empty() { return this->empty; }
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

     inline execution_slot* get_executor(const int exec) { return this->execution_slots[exec]; }

     ~control_store_module();

  };
  
}

#endif
