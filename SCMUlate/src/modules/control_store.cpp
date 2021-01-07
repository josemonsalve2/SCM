#include "control_store.hpp"

void 
scm::execution_slot::insert(scm::instruction_state_pair *newInstruction) {
  *this->tail = newInstruction;
  getNext(tail);
}

void 
scm::execution_slot::consume() {
  (*head)->second = instruction_state::EXECUTION_DONE;
  *this->head = nullptr;
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
