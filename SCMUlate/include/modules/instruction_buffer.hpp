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
 *  DECOMMISSION: The instruction has been finished, and all cleaning has been taken care of.
 *              The buffer can recovery the buffer slot.
 *  STALL: The PC should not fetch any more instructions. The next instruction has not been 
 *         determined yet. 
 */

#include "SCMUlate_tools.hpp"
#include "instruction_buffer.hpp"
#include "instructions.hpp"
#include <deque>
#include <map>
#include <utility>

namespace scm {

  class instructions_buffer_module {
    private:
      std::deque <instruction_state_pair *> instruction_buffer;
      std::map<instruction_state_pair *, dupCodeletStates> duplicatedCodelets;
      std::map<instruction_state_pair *, instruction_state_pair *>
          reverseDuplicatedCodelets;

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

      void insertDuplicatedCodelets(instruction_state_pair *original,
                                    instruction_state_pair *duplicated) {
        this->duplicatedCodelets[original].push_back(duplicated);
        this->reverseDuplicatedCodelets[duplicated] = original;
        this->instruction_buffer.push_back(duplicated);
        SCMULATE_INFOMSG(5,
                         "Adding Duplicated codelet Instruction %s at (0x%lX) to container of duplicates. Original is of %s at (0x%lX)",
                         duplicated->first->getFullInstruction().c_str(), (unsigned long)duplicated, original->first->getFullInstruction().c_str(), (unsigned long)original);
      }

      bool isReplica(instruction_state_pair *instruction) {
        return this->reverseDuplicatedCodelets.find(instruction) !=
               this->reverseDuplicatedCodelets.end();
      }

      instruction_state_pair *getOriginal(instruction_state_pair *duplicated) {
        SCMULATE_ERROR_IF(0, !this->isReplica(duplicated),
                          "Trying to get original codelet %s from a"
                          " codelet that is not a replica",
                          duplicated->first->getFullInstruction().c_str());
        return this->reverseDuplicatedCodelets[duplicated];
      }


      bool isDuplicated(instruction_state_pair *instruction) {
        return this->duplicatedCodelets.find(instruction) !=
               this->duplicatedCodelets.end();
      }

      dupCodeletStates *getDuplicated(instruction_state_pair *original) {
        for (auto dup: this->duplicatedCodelets)
          SCMULATE_INFOMSG(10, "%ld Duplicates of codelets of %s at (0x%lX)",
                         dup.second.size(), dup.first->first->getFullInstruction().c_str(), (unsigned long)dup.first);
        SCMULATE_ERROR_IF(0, !this->isDuplicated(original),
                          "Trying to get duplicated codelet %s at (0x%lX) from a non "
                          "duplicated codelet",
                          original->first->getFullInstruction().c_str(), (unsigned long)original);
        return &this->duplicatedCodelets[original];
      }

      /// @brief Check if all the copies are ready for comparison
      /// @param original original codelet. Will check over all the duplicates
      /// @return
      bool checkIfReady(instruction_state_pair *original) {
        if (original->second != EXECUTION_DONE)
          return false;
        for (auto copies : this->duplicatedCodelets[original])
          if (copies->second != EXECUTION_DONE_DUP)
            return false;
        return true;
      }

      void removeDuplicates(instruction_state_pair *original) {
        SCMULATE_INFOMSG(5, "Deleting Instruction %s from duplication", original->first->getFullInstruction().c_str());
        for (auto copy : this->duplicatedCodelets[original]) {
          SCMULATE_INFOMSG(5,
                           "Deleting Duplicated codelet Instruction %s from",
                           copy->first->getFullInstruction().c_str());
          this->reverseDuplicatedCodelets.erase(copy);
        }
        this->duplicatedCodelets.erase(original);
      }

      void clean_out_queue() {
        for (auto it = instruction_buffer.begin(); it != instruction_buffer.end() ;) {
          if ((*it)->second == instruction_state::DECOMMISSION) {
            SCMULATE_INFOMSG(5, "Deleting Instruction %s at %p from buffer",
                             (*it)->first->getFullInstruction().c_str(), *it);
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

      uint64_t inline getBufferSize() const {
        return this->instruction_buffer.size();
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
