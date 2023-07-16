#include "control_store.hpp"

bool scm::execution_slot::try_insert(
    scm::instruction_state_pair *newInstruction) {

  if ((*tail) == nullptr) {
    auto t = this->tail;
#pragma omp atomic write
    *t = newInstruction;
    getNext(tail);
#pragma omp flush
    return true;
  } else {
    return false;
  }
}

void 
scm::execution_slot::consume() {
  auto &to_end = (*head)->second;

  instructions_queue_t h;
#pragma omp atomic read
  h = this->head;
#pragma omp atomic write
  *h = nullptr;
  getNext(head);
#pragma omp flush
  to_end = instruction_state::EXECUTION_DONE;
}

scm::control_store_module::control_store_module(const int numExecUnits) {
  // Creating all the execution slots
  for (int i = 0; i < numExecUnits; i ++) {
    this->execution_slots.push_back(new execution_slot());
  }
}

scm::control_store_module::~control_store_module() {
  // Deleting the execution slots
  for (auto it = this->execution_slots.rbegin();
       it != this->execution_slots.rend(); ++it)
    delete (*it);
}
