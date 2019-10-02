#include "scm_machine.hpp"

scm::scm_machine::scm_machine(std::string in_filename):
  alive(false), 
  init_correct(true), 
  filename(in_filename),
  reg_file_m(),
  inst_mem_m(filename), 
  fetch_decode_m(&inst_mem_m) {
    SCMULATE_INFOMSG(0, "Initializing SCM machine")
  
    // We check the register configuration is valid
    if(!reg_file_m.checkRegisterConfig()) {
      SCMULATE_ERROR(0, "Error when checking the register");
      init_correct = false;
      return;
    }
  
    init_correct = true;
}

scm::run_status
scm::scm_machine::run() {
  if (!this->init_correct) return SCM_RUN_FAILURE;
  this-> alive = true;
  int run_result = 0;
#pragma omp parallel reduction(+: run_result)
  {
    #pragma omp master 
    {
      SCMULATE_INFOMSG(1, "Running with %d threads ", omp_get_num_threads());
    }
    switch (omp_get_thread_num()) {
      case SU_THREAD:
        fetch_decode_m.behavior();
        break;
      case MEM_THREAD:
//        run_result = SSCMUlate_MEM_Behavior();
        break;
      case CU_THREADS:
//        run_result = SSCMUlate_CU_Behavior();
        break;
      default:
        SCMULATE_WARNING(0, "Thread created with no purpose. What's my purpose? You pass the butter");
    }
    #pragma omp barrier 

  }

  if (run_result != 0 ) return SCM_RUN_FAILURE;
  return SCM_RUN_SUCCESS;

}
