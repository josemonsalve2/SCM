#include "control_store.hpp"

void 
scm::execution_slot::assign(scm::decoded_instruction_t *newInstruction) {
  // TODO: Where is the codelet created and where is it destroid? we might want to 
  //       have this as pointers instead of make a codelet copy every time
  this->executionInstruction = newInstruction;
  #pragma omp atomic write
  this->empty = false;
}

void 
scm::execution_slot::empty_slot() {
  #pragma omp atomic write
  this->empty = true;
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
