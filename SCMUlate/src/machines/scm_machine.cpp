#include "scm_machine.hpp"
#include <iostream>

scm::scm_machine::scm_machine(char * in_filename, l2_memory_t const memory, ILP_MODES ilp_mode):
  alive(false), 
  init_correct(true), 
  filename(in_filename),
  reg_file_m(),
  inst_mem_m(filename, &reg_file_m), 
  control_store_m(NUM_CUS),
  fetch_decode_m(&inst_mem_m, &control_store_m, &alive, ilp_mode) {
    SCMULATE_INFOMSG(0, "Initializing SCM machine")
    // Configuration parameters
  
    // We check the register configuration is valid
    if(!reg_file_m.checkRegisterConfig()) {
      SCMULATE_ERROR(0, "Error when checking the register");
      init_correct = false;
      return;
    }
    
    if(!inst_mem_m.isValid()) {
      SCMULATE_ERROR(0, "Error when loading file");
      init_correct = false;
      return;
    }

    TIMERS_COUNTERS_GUARD(
      this->fetch_decode_m.setTimerCounter(&this->time_cnt_m);
      this->time_cnt_m.addTimer("SCM_MACHINE",scm::SYS_TIMER);
    )

    // Creating execution Units
    int exec_units_threads[] = {CUS};
    int * cur_exec = exec_units_threads;
    for (int i = 0; i < NUM_CUS; i++, cur_exec++) {
      SCMULATE_INFOMSG(4, "Creating executor (CUMEM) %d out of %d for thread %d", i, NUM_CUS, *cur_exec);
      cu_executor_module* newExec = new cu_executor_module(*cur_exec, &control_store_m, i, &alive, memory);
      TIMERS_COUNTERS_GUARD(
        newExec->setTimerCnt(&this->time_cnt_m);
      )
      executors_m.push_back(newExec);
    }
      
    init_correct = true;
    ITT_RESUME;
}

scm::run_status
scm::scm_machine::run() {
  if (!this->init_correct) return SCM_RUN_FAILURE;
  TIMERS_COUNTERS_GUARD(
    this->time_cnt_m.resetTimer();
    this->time_cnt_m.addEvent("SCM_MACHINE",SYS_START);
  );
  this->alive = true;
  int run_result = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> timer = std::chrono::high_resolution_clock::now();
#pragma omp parallel reduction(+: run_result) shared(alive) num_threads(NUM_CUS+1)
  {
    #pragma omp master 
    {
      SCMULATE_INFOMSG(1, "Running with %d threads ", omp_get_num_threads());
    }
    switch (omp_get_thread_num()) {
      case SU_THREAD:
        fetch_decode_m.behavior();
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
        {
          // Initialization barrier
          #pragma omp barrier 
          SCMULATE_WARNING(0, "Thread created with no purpose. What's my purpose? You pass the butter");
        }
    }
    #pragma omp barrier 

  }
  std::chrono::time_point<std::chrono::high_resolution_clock> timer2 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = timer2 - timer;
  std::cout << "Exec Time = " << diff.count() << std::endl;
  TIMERS_COUNTERS_GUARD(
    this->time_cnt_m.addEvent("SCM_MACHINE",SYS_END);
  );

  if (run_result != 0 ) return SCM_RUN_FAILURE;
  return SCM_RUN_SUCCESS;

}

scm::scm_machine::~scm_machine() {
  ITT_PAUSE;
  for (auto it = executors_m.begin(); it < executors_m.end(); ++it) 
    delete (*it);
  TIMERS_COUNTERS_GUARD(
    this->time_cnt_m.dumpTimers();
  );
}
