#ifndef __CONTROL_STORE__
#define __CONTROL_STORE__

#include "SCMUlate_tools.hpp"
#include "codelet.hpp"
#include "instructions.hpp"
#include "threads_configuration.hpp"
#include <vector>

namespace scm {

/* An execution slot connects the scheduling of an instruction to its
 * corresponding executor. The executor will be reading the execution
 * slot waiting for instructions to be available, once there's something
 * it will start its execution, and when it is finished it will remove it
 * At this stage it's assumed the result has been commited either to the memory
 * unit or or the register directly. On the other hand the scheduler will have
 * a list of all the execution slots and chose one according to some
 * scheduling policy.
 */
typedef enum { EMPTY, BUSY, DONE } executorState;
typedef instruction_state_pair **instructions_queue_t;
class execution_slot {
private:
  instructions_queue_t executionQueue;
  instructions_queue_t head;
  instructions_queue_t tail;

public:
  // Constructor
  execution_slot() {
    executionQueue = new instruction_state_pair *[EXECUTION_QUEUE_SIZE];
    for (int i = 0; i < EXECUTION_QUEUE_SIZE; i++) {
      executionQueue[i] = nullptr;
    }
    head = tail = executionQueue;
  };

  bool try_insert(instruction_state_pair *);
  void consume();

  inline bool is_empty() {
    instructions_queue_t h, t;
    instruction_state_pair *tmp;
#pragma omp flush
#pragma omp atomic read
    h = head;
#pragma omp atomic read
    t = tail;
#pragma omp atomic read
    tmp = *h;
    return h == t && tmp == nullptr;
  }

  void inline getNext(instructions_queue_t &current) {
    auto next = current;
    next++;

    if (next == executionQueue + EXECUTION_QUEUE_SIZE)
      next = executionQueue;
#pragma omp atomic write
    current = next;
    SCMULATE_INFOMSG(5, "Getting next address %p", current);
  }

  inline instruction_state_pair *getHead() const {
    instructions_queue_t h;
#pragma omp atomic read
    h = head;
    SCMULATE_INFOMSG(5, "Getting head %p", h);
    return *h;
  }

  ~execution_slot() { delete[] executionQueue; }
};

/* Control store corresponds to the glue logic between the different
 * modules and the execution of an instruction. The control logic
 * should contain the different execution slots and any additional logic
 * that allows the connection between the modules that schedule the
 * instructions and those that compute. Execution slots are created and
 * deleted here
 */
class control_store_module {
private:
  std::vector<execution_slot *> execution_slots;

public:
  control_store_module() = delete;
  control_store_module(const int numExecUnits);

  inline execution_slot *get_executor(const int exec) const {
    return this->execution_slots[exec];
  }
  inline uint32_t numExecutors() { return execution_slots.size(); }

  ~control_store_module();
};

} // namespace scm

#endif
