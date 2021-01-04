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
      std::deque <instruction_state_pair *> instruction_buffer;
    public: 
      // TODO: this may not be the best way of doing this. The position will change if the 
      // elements in the front of the queue change. Therefore it cannot be used as 
      // a reference in the subscription.
      // Because of this we will need to use a vector in the subscribers of ILP with a pointer
      // to the actual instruction_state_pair
      instructions_buffer_module() { }

      bool add_instruction(decoded_instruction_t & new_instruction) {
        // Check if we have reached the limit size
        if (isBufferFull() || (this->instruction_buffer.size() != 0  && this->instruction_buffer.back()->second == STALL))
          return false;
        instruction_state_pair * newPair = new instruction_state_pair(new decoded_instruction_t(new_instruction), WAITING);
        // Avoid calling copy constructor twice
        SCMULATE_INFOMSG(5, "Adding Instruction %s to buffer", newPair->first->getFullInstruction().c_str());
        this->instruction_buffer.push_back(newPair); // Call constructor
        return true;
      }

      void clean_out_queue() {
        for (auto it = instruction_buffer.begin(); it != instruction_buffer.end() ;) {
          if ((*it)->second == instruction_state::DECOMISION) {
            SCMULATE_INFOMSG(5, "Deleting Instruction %s from buffer", (*it)->first->getFullInstruction().c_str());
            delete (*it)->first;
            delete *it;
            it = instruction_buffer.erase(it);
          } else {
            ++it;
          }
        }
      }

      bool isBufferFull() {
        return this->instruction_buffer.size() >= INSTRUCTIONS_BUFFER_SIZE;
      }

      std::deque <instruction_state_pair *> * get_buffer() { return &this->instruction_buffer; }
      instruction_state_pair* get_latest() { return this->instruction_buffer.back(); }

      ~instructions_buffer_module () {
        for (auto it = instruction_buffer.begin(); it != instruction_buffer.end() ;) {
          SCMULATE_INFOMSG(5, "Deleting Instruction %s from buffer", (*it)->first->getFullInstruction().c_str());
          delete (*it)->first;
          delete *it;
          it = instruction_buffer.erase(it);
        }
      }
  };
}

#endif
