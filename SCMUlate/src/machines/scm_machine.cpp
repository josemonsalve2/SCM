#include "scm_machine.hpp"

scm::scm_machine::scm_machine(std::string in_filename):
  alive(false), 
  init_correct(true), 
  filename(in_filename),
  reg_file_m(),
  inst_mem_m(filename), 
  control_store_m(NUM_CUS),
  fetch_decode_m(&inst_mem_m, &control_store_m, &reg_file_m, &alive) {
    SCMULATE_INFOMSG(0, "Initializing SCM machine")
  
    // We check the register configuration is valid
    if(!reg_file_m.checkRegisterConfig()) {
      SCMULATE_ERROR(0, "Error when checking the register");
      init_correct = false;
      return;
    }

    // Creating execution Units
    int exec_units_threads[] = {CUS};
    int * cur_exec = exec_units_threads;
    for (int i = 0; i < NUM_CUS; i++, cur_exec++) {
      SCMULATE_INFOMSG(4, "Creating executor %d out of %d for thread %d", i, NUM_CUS, *cur_exec);
      executors_m.push_back( new cu_executor_module(*cur_exec, &control_store_m, i, &alive) );
    }
      
    init_correct = true;
}

scm::run_status
scm::scm_machine::run() {
  if (!this->init_correct) return SCM_RUN_FAILURE;
  this->alive = true;
  int run_result = 0;
#pragma omp parallel reduction(+: run_result) shared(alive)
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
        // Find my executor
        for (auto it = executors_m.begin(); it < executors_m.end(); ++it) {
          if ((*it)->get_executor_id() == omp_get_thread_num()) {
            (*it)->behavior();
            break; // exit for loop
          }
        }
        break;
      default:
        SCMULATE_WARNING(0, "Thread created with no purpose. What's my purpose? You pass the butter");
    }
    #pragma omp barrier 

  }

  if (run_result != 0 ) return SCM_RUN_FAILURE;
  return SCM_RUN_SUCCESS;

}

scm::scm_machine::~scm_machine() {
  for (auto it = executors_m.begin(); it < executors_m.end(); ++it) 
    delete (*it);
}
