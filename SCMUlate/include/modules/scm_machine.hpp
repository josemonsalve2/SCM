#ifndef __MACHINE_CONFIGURATION__
#define __MACHINE_CONFIGURATION__

#include "SCMUlate_tools.hpp"
#include "register.h"

#include <omp.h>
namespace scm{

  enum run_status_t {SCM_RUN_SUCCESS, SCM_RUN_FAILURE};
  typedef enum run_status_t run_status;

  class scm_machine {
    bool alive;
    bool init_correct;
    char filename[100];
    
    // Modules
    reg_file_module reg_file_m;
    
  
    scm_machine() = delete;
    scm_machine(char filename); 
    run_status run();
  
    ~scm_machine();
  
  };
}

#endif //__MACHINE_CONFIGURATION__
