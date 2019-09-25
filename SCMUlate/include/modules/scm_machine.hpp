#ifndef __MACHINE_CONFIGURATION__
#define __MACHINE_CONFIGURATION__

#include "SCMUlate_tools.hpp"
#include "threads_configuration.hpp"
#include "register.hpp"
#include "instruction_mem.hpp"
#include <omp.h>
#include <string>

namespace scm{

  enum run_status_t {SCM_RUN_SUCCESS, SCM_RUN_FAILURE};
  typedef enum run_status_t run_status;

  class scm_machine {
    private: 
      bool alive;
      bool init_correct;
      std::string filename;
    
      // Modules
      reg_file_module reg_file_m;
      inst_mem_module inst_mem_m;
    
    public: 
      scm_machine() = delete;
      scm_machine(std::string in_filename); 
      run_status run();
    
      ~scm_machine() {};
  
  };
}

#endif //__MACHINE_CONFIGURATION__
