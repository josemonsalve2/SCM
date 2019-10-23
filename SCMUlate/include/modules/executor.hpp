#ifndef __EXECUTOR__
#define __EXECUTOR__

#include "SCMUlate_tools.hpp"
#include "control_store.hpp"

namespace scm {

  class cu_executor_module{
    private:
      execution_slot * myExecutor;
      bool* aliveSignal;

    public: 
      cu_executor_module() = delete;
      cu_executor_module(control_store_module * const, unsigned int, bool *);

      int behavior();
  
  };
  
}

#endif
