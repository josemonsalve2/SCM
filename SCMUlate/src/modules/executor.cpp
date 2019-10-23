#include "executor.hpp"

scm::cu_executor_module(control_store_module * const control_store_m, unsigned int execNumber, bool * aliveSig):
  aliveSignal(aliveSig) {
    this->myExecutor = control_store_m->getExecutor(execNumber);
}

int
scm::cu_executor_module::behavior() {
  while (*(this->aliveSignal)) {
    if (!myExecutor->isEmpty) {
      // execute the codelet here
    }
  }
}
