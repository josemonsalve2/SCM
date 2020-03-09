#include "executor.hpp"

scm::cu_executor_module::cu_executor_module(int CU_ID, control_store_module * const control_store_m, unsigned int execSlotNumber, bool * aliveSig):
  cu_executor_id(CU_ID),
  aliveSignal(aliveSig) {
    this->myExecutor = control_store_m->get_executor(execSlotNumber);
}

int
scm::cu_executor_module::behavior() {
  SCMULATE_INFOMSG(1, "Starting CU %d behavior", cu_executor_id);
  // Initialization barrier
  #pragma omp barrier
  while (*(this->aliveSignal)) {
    if (!myExecutor->is_empty()) {
      SCMULATE_INFOMSG(4, "  CU[%d]: Executing codelet ", cu_executor_id);
      scm::codelet * curCodelet = myExecutor->getHead();
      curCodelet->implementation();
      // TODO Delete args? 
      unsigned char ** curArgs = static_cast<unsigned char **>(curCodelet->getParams());
      delete curArgs;
      delete curCodelet;
      myExecutor->empty_slot();
    }
  }
  SCMULATE_INFOMSG(1, "Shutting down executor CU %d", cu_executor_id);
  return 0;
}
 
