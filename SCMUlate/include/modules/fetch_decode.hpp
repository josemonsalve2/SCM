#ifndef __FETCH_DECODE__
#define __FETCH_DECODE__

#include "SCMUlate_tools.hpp"
#include "instruction_mem.hpp"

namespace scm {

  class fetch_decode_module {
    private:
      inst_mem_module * inst_mem_m;
      bool * alive; 
      int PC; 
  
      fetch_decode_module() = delete;
      fetch_decode_module(inst_mem_module * const inst_mem);

      int behavior();
  
    public: 
  }
  
}

#endif
