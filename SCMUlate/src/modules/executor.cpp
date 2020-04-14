#include "executor.hpp"

scm::cu_executor_module::cu_executor_module(int CU_ID, control_store_module * const control_store_m, unsigned int execSlotNumber, bool * aliveSig, l2_memory_t upperMem):
  cu_executor_id(CU_ID),
  aliveSignal(aliveSig) {
    this->myExecutor = control_store_m->get_executor(execSlotNumber);
    this->mem_interface_t = new mem_interface_module(upperMem);
}

int
scm::cu_executor_module::behavior() {
  TIMERS_COUNTERS_GUARD(
    this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_START);
  );
  SCMULATE_INFOMSG(1, "Starting CUMEM %d behavior", cu_executor_id);
  // Initialization barrier
  #pragma omp barrier
  while (*(this->aliveSignal)) {
    if (myExecutor->is_busy()) {
      SCMULATE_INFOMSG(4, "  CUMEM[%d]: Executing instruction ", cu_executor_id);
      scm::decoded_instruction_t * curInstruction = myExecutor->getHead();
      if (curInstruction->getType()== scm::instType::MEMORY_INST) {
        TIMERS_COUNTERS_GUARD(
          this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_EXECUTION_MEM);
        );
        this->mem_interface_t->assignInstSlot(curInstruction);
        this->mem_interface_t->behavior();
      } else if (curInstruction->getType() == scm::instType::EXECUTE_INST) {
        TIMERS_COUNTERS_GUARD(
          this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_EXECUTION_COD);
        );
        codeletExecutor();
      } else {
        SCMULATE_ERROR(0, "Error. Executor received an unknown instruction type");
      }
      
      myExecutor->done_execution();
      TIMERS_COUNTERS_GUARD(
        this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_IDLE);
      );
    }
  }
  SCMULATE_INFOMSG(1, "Shutting down executor CUMEM %d", cu_executor_id);
  TIMERS_COUNTERS_GUARD(
    this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_END);
  );
  return 0;
}
 

int scm::cu_executor_module::codeletExecutor() {
    scm::decoded_instruction_t * curInstruction = myExecutor->getHead();
    scm::codelet * curCodelet = curInstruction->getExecCodelet();
    curCodelet->implementation();
    return 0;
}