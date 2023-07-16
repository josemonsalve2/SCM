#include "executor.hpp"

scm::cu_executor_module::cu_executor_module(int CU_ID, control_store_module * const control_store_m, unsigned int execSlotNumber, bool * aliveSig, l2_memory_t upperMem):
  cu_executor_id(CU_ID),
  aliveSignal(aliveSig) {
    this->myExecutor = control_store_m->get_executor(execSlotNumber);
    this->mem_interface_t = new mem_interface_module(upperMem, this);
}

int
scm::cu_executor_module::behavior() {
  TIMERS_COUNTERS_GUARD(
    #ifdef PAPI_COUNT
    this->timer_cnt_m->initPAPIcounter(this->cu_timer_name);
    #endif
    this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_START);
  );
  SCMULATE_INFOMSG(1, "Starting CUMEM %d behavior", cu_executor_id);
  // Initialization barrier
  #pragma omp barrier
  while (*(this->aliveSignal)) {
    if (!myExecutor->is_empty()) {
      SCMULATE_INFOMSG(4, "  CUMEM[%d]: Executing instruction ", cu_executor_id);
      scm::decoded_instruction_t * curInstruction = myExecutor->getHead()->first;
      printf("  CUMEM[%d]: Executing instruction %s\n", cu_executor_id, curInstruction->getFullInstruction().data());
      if (curInstruction->getType() == scm::instType::MEMORY_INST || curInstruction->getExecCodelet()->isMemoryCodelet()) {
        TIMERS_COUNTERS_GUARD(
          #ifdef PAPI_COUNT
          this->timer_cnt_m->startPAPIcounters(this->cu_timer_name);
          #endif
          this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_EXECUTION_MEM, curInstruction->getFullInstruction());
        );
        this->mem_interface_t->assignInstSlot(curInstruction);
        this->mem_interface_t->behavior();
      } else if (curInstruction->getType() == scm::instType::EXECUTE_INST) {
        TIMERS_COUNTERS_GUARD(
          #ifdef PAPI_COUNT
          this->timer_cnt_m->startPAPIcounters(this->cu_timer_name);
          #endif
          this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_EXECUTION_COD, curInstruction->getFullInstruction());
        );
        codeletExecutor();
      } else {
        SCMULATE_ERROR(0, "Error. Executor received an unknown instruction type");
      }
      
      TIMERS_COUNTERS_GUARD(
        #ifdef PAPI_COUNT
          timer_event& anEvent = this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_IDLE);
          this->timer_cnt_m->stopAndRegisterPAPIcounters(this->cu_timer_name, anEvent);
        #else 
          this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_IDLE);
        #endif
      );
      myExecutor->consume();
    }
  }
  SCMULATE_INFOMSG(1, "Shutting down executor CUMEM %d", cu_executor_id);
  TIMERS_COUNTERS_GUARD(
    this->timer_cnt_m->addEvent(this->cu_timer_name, CUMEM_END);
  );
  return 0;
}
 

int scm::cu_executor_module::codeletExecutor() {
    scm::decoded_instruction_t * curInstruction = myExecutor->getHead()->first;
    scm::codelet * curCodelet = curInstruction->getExecCodelet();
    curCodelet->setExecutor(this);
    curCodelet->implementation();
    return 0;
}