#ifndef __MACHINE_CONFIGURATION__
#define __MACHINE_CONFIGURATION__

#include <omp.h>
#include <string>

// SCM Related Includes
#include "SCMUlate_tools.hpp"
#include "threads_configuration.hpp"
#include "register.hpp"
#include "control_store.hpp"
#include "executor.hpp"
#include "instruction_mem.hpp"
#include "fetch_decode.hpp"
#include "memory_interface.hpp"
#include "system_codelets.hpp"
#include "timers_counters.hpp"


namespace scm {

  enum run_status_t {SCM_RUN_SUCCESS, SCM_RUN_FAILURE};
  typedef enum run_status_t run_status;

  class scm_machine {
    private: 
      bool alive;
      bool init_correct;
      std::string filename;
      TIMERS_COUNTERS_GUARD(timers_counters time_cnt_m;)
      
      // Modules
      reg_file_module reg_file_m;
      inst_mem_module inst_mem_m;
      control_store_module control_store_m;
      fetch_decode_module fetch_decode_m;
      mem_interface_module mem_interface_m;
      std::vector<cu_executor_module*> executors_m;

    public: 
      scm_machine() = delete;
      scm_machine(std::string in_filename, unsigned char * const external_memory); 

      TIMERS_COUNTERS_GUARD( 
        void inline setTimersOutput(std::string outputName) { this->time_cnt_m.setFilename(outputName); }
      )

      run_status run();
    
      ~scm_machine();
  
  };
}

#endif //__MACHINE_CONFIGURATION__
