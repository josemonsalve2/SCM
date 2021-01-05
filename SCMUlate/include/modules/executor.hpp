#ifndef __EXECUTOR_MODULE__
#define __EXECUTOR_MODULE__

#include "SCMUlate_tools.hpp"
#include "control_store.hpp"
#include "timers_counters.hpp"
#include "memory_interface.hpp"

namespace scm {

  /** brief: Executors 
   *
   *  The cu_executor performs the execution of instructions, usually in
   *  the form of codelets. It requires an execution slot which contains
   *  the codelet that is to be executed.*/
  class cu_executor_module {
    private:
      int cu_executor_id;
      execution_slot * myExecutor;
      mem_interface_module *mem_interface_t;
      volatile bool* aliveSignal;
      TIMERS_COUNTERS_GUARD(
        std::string cu_timer_name;
        timers_counters* timer_cnt_m;
      )
      

    public: 
      cu_executor_module() = delete;
      cu_executor_module(int, control_store_module * const, unsigned int, bool *, l2_memory_t upperMem);

      TIMERS_COUNTERS_GUARD(
        inline void setTimerCnt(timers_counters * tmc) { 
          this->timer_cnt_m = tmc;
          this->cu_timer_name = std::string("CUMEM_") + std::to_string(this->cu_executor_id);
          this->timer_cnt_m->addTimer(this->cu_timer_name, CUMEM_TIMER);
        }
      )

      int behavior();
      int codeletExecutor();

      int get_executor_id(){ return this->cu_executor_id; }
      mem_interface_module* get_mem_interface() { return this->mem_interface_t; }

      ~cu_executor_module() {
        delete mem_interface_t;
      }
  
  };
  
}

#endif
