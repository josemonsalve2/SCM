#ifndef __EXECUTOR_MODULE__
#define __EXECUTOR_MODULE__

#include "SCMUlate_tools.hpp"
#include "control_store.hpp"

namespace scm {

  /* The cu_executor performs the execution of instructions, usually in
   * the form of codelets. It requiers an execution slot which contains
   * the codelet that is to be executed.*/
  class cu_executor_module{
    private:
      int cu_executor_id;
      execution_slot * myExecutor;
      volatile bool* aliveSignal;

    public: 
      cu_executor_module() = delete;
      cu_executor_module(int, control_store_module * const, unsigned int, bool *);

      int behavior();

      int get_executor_id(){ return this->cu_executor_id; };
  
  };
  
}

#endif
