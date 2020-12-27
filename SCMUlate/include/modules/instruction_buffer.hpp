#ifndef __INSTRUCTION_BUFFER_DECODE__
#define __INSTRUCTION_BUFFER_DECODE__

/** \brief Instruction buffer for the fetch/decode and ILP
 * 
 *  The fetch decode execution uses a buffer to store the instructions that are
 *  near the current program counter. It represents the instruction window. 
 *  The instructions are copied over the window and marked with a state
 *  depending on the ILP mode (e.g. sequential, superscalar or Out of Order).
 *  
 *  The decoded instruction is marked through a pair: instruction + state.
 *  The current states are:
 *  WAITING: The instruction is waiting for structural or data dependencies
 *  READY: The instruction is ready to be scheduled and executed. The SU (fetch decode)
 *          will determine when to execute it
 *  DONE: The instruction has been finished, there may be some cleaning that needs to be handled.
 *  DECOMISION: The instruction has been finished, and all cleaning has been taken care of.
 *              The buffer can recovery the buffer slot.
 *  STALL: The PC should not fetch any more instructions. The next instruction has not been 
 *         determined yet. 
 */

#include "SCMUlate_tools.hpp"
#include "instructions.hpp"
#include "instruction_buffer.hpp"
#include <deque>
#include <utility>

namespace scm {

  class instructions_buffer_module {
    private:
      std::deque <instruction_state_pair> instruction_buffer;
    public: 
      // TODO: this may not be the best way of doing this. The position will change if the 
      // elements in the front of the queue change. Therefore it cannot be used as 
      // a reference in the subscription.
      // Because of this we will need to use a vector in the subscribers of ILP with a pointer
      // to the actual instruction_state_pair
      instructions_buffer_module() { }

      bool add_instruction(decoded_instruction_t & new_instruction) {
        // Check if we have reached the limit size
        if (this->instruction_buffer.size() >= INSTRUCTIONS_BUFFER_SIZE || (this->instruction_buffer.size() != 0  && this->instruction_buffer.back().second == STALL))
          return false;
        this->instruction_buffer.emplace_back(instruction_state_pair(new_instruction, WAITING)); // Call constructor
        return true;
      }

      void clean_out_queue() {
        for (auto it = instruction_buffer.begin(); it != instruction_buffer.end() ;) {
          if (it->second == DECOMISION) {
            it = instruction_buffer.erase(it);  
          } else {
            ++it;
          }
        }
      }

      std::deque <instruction_state_pair> * get_buffer() { return &this->instruction_buffer; }
      instruction_state_pair* get_latest() { return &this->instruction_buffer.back(); }
  };
}

#endif
