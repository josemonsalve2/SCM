#ifndef __CONTROL_STORE__
#define __CONTROL_STORE__

#include "SCMUlate_tools.hpp"
#include "threads_configuration.hpp"
#include "codelet.hpp"

namespace scm {

  class execution_slot {
    codelet executionCodelet();
  
    void assing(codelet);
    
  }

  class control_store_module{
    private:

    public: 
     control_store_module() = delete;
//     control_store_module();

  };
  
}

#endif
