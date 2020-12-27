#include "control_store.hpp"

void 
scm::execution_slot::assign(scm::instruction_state_pair *newInstruction) {
  this->executionInstruction = newInstruction;
  #pragma omp atomic write
  this->state = BUSY;
}

void 
scm::execution_slot::empty_slot() {
  this->executionInstruction = NULL;
  #pragma omp atomic write
  this->state = EMPTY;
}

void 
scm::execution_slot::done_execution() {
  #pragma omp atomic write
  this->state = DONE;
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
