#ifndef __MACHINE_CONFIGURATION__
#define __MACHINE_CONFIGURATION__

#include <omp.h>
#include <string>

// SCM Related Includes
#include "SCMUlate_tools.hpp"
#include "system_config.hpp"
#include "threads_configuration.hpp"
#include "register.hpp"
#include "control_store.hpp"
#include "executor.hpp"
#include "instruction_mem.hpp"
#include "fetch_decode.hpp"
#include "system_codelets.hpp"
#include "timers_counters.hpp"


namespace scm {

  enum run_status_t {SCM_RUN_SUCCESS, SCM_RUN_FAILURE};
  typedef enum run_status_t run_status;

  class scm_machine {
    private: 
      bool alive;
      bool init_correct;
      char* filename;
      TIMERS_COUNTERS_GUARD(timers_counters time_cnt_m;)
      
      // Modules
      reg_file_module reg_file_m;
      inst_mem_module inst_mem_m;
      control_store_module control_store_m;
      fetch_decode_module fetch_decode_m;
      std::vector<cu_executor_module*> executors_m;

    public: 
      scm_machine() = delete;
      scm_machine(char * in_filename, unsigned char * const external_memory, ILP_MODES ilp_mode = ILP_MODES::SEQUENTIAL); 

      // getters
      inline reg_file_module * getRegFile() {return &reg_file_m; }
      inline inst_mem_module * getInstMemory() { return &inst_mem_m; }
      inline control_store_module * getControlStore() { return &control_store_m; }
      inline fetch_decode_module * getFetchDecode() { return &fetch_decode_m; }
      inline cu_executor_module * getExecutorCU (uint32_t execID) { return executors_m[execID]; }

      TIMERS_COUNTERS_GUARD( 
        void inline setTimersOutput(std::string outputName) { this->time_cnt_m.setFilename(outputName); }
      )

      run_status run();
    
      ~scm_machine();
  
  };
}

#endif //__MACHINE_CONFIGURATION__
