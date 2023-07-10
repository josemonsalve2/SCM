#include "control_store.hpp"

bool 
scm::execution_slot::try_insert(scm::instruction_state_pair *newInstruction) {
  #pragma omp flush
  if ((*tail) == nullptr) {
    *this->tail = newInstruction;
    getNext(tail);
    return true;
  } else {
    return false;
  }

}

void 
scm::execution_slot::consume() {
  if ((*head)->second == instruction_state::EXECUTING_DUP)
    (*head)->second = instruction_state::EXECUTION_DONE_DUP;  
  else
    (*head)->second = instruction_state::EXECUTION_DONE;

  *this->head = nullptr;
  #pragma omp flush release 
  getNext(head);
}

scm::control_store_module::control_store_module(const int numExecUnits) {
  // Creating all the execution slots
  for (int i = 0; i < numExecUnits; i ++) {
    this->execution_slots.push_back(new execution_slot());
  }
}

scm::control_store_module::~control_store_module() {
  // Deleting the execution slots
  for (auto it = this->execution_slots.rbegin(); it < this->execution_slots.rend(); ++it)
    delete (*it);

}
