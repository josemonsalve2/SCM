#include "ilp_controller.hpp"

namespace scm {

        bool 
        ilp_OoO::checkMarkInstructionToSched(instruction_state_pair * inst_state) {

        decoded_instruction_t * inst = (inst_state->first);

        // COMMIT INSTRUCTIONS they are handled differently. 
        // No operands, but everything must have finished. Empty pipeline
        if (inst->getType() == instType::COMMIT) {
          if (used.size() != 0 || memCtrl.numberOfRanges() != 0) {
            SCMULATE_INFOMSG_IF(5, inst_state->second != instruction_state::STALL, "Stalling on instruction %s", inst->getFullInstruction().c_str());
            inst_state->second = instruction_state::STALL;
            return false;
          } else {
            inst_state->second = instruction_state::READY;
            return true;
          }
        }

        // If there is a structural hazzar, we must solve it first
        if (hazzard_inst_state != nullptr) {
          inst_state = hazzard_inst_state;
          inst = inst_state->first;
          SCMULATE_INFOMSG(5, "Trying to scheudule instruction %s again", inst->getFullInstruction().c_str());
        } else if (inst->isMemoryInstruction() && inst_state->second == instruction_state::STALL) {
          // MEMORY INSTRUCTIONS. If they are stalled, it is because they have an unsatisfied dependency
          // that may be a missing memory address, or a conflict with a memory access from other insturction
          // Therefore we must check if this is still the case
          // In memory instructions or memory codelets we need to figure out if there is a hazard in the memory
          if (stallMemoryInstruction(inst))
            return false;
          if (isInstructionReady(inst)) {
            inst_state->second = instruction_state::READY;
            return true;
          } else {
            // Address has been calculated, but maybe other operands are still waiting
            inst_state->second = instruction_state::WAITING;
            return false;
          }
          return false;
        } else if (reservationTable.find(inst_state) != reservationTable.end()) {
          // Check if we have already processed the instructions. If so, just return
          return false;
        } else {
          already_processed_operands.clear();
          reservationTable.insert(inst_state); 
        }
        

        SCMULATE_INFOMSG(3, "Dependency discovery and hazzard avoidance process on instruction (%lu) %s", (unsigned long) inst, inst->getFullInstruction().c_str());
        // Get directions for the current instruction. (Col 1)
        getOperandsDirs(inst);
        
        // Let's analyze each operand's dependency. Marking as allow sched or not.
        for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
          if (already_processed_operands.find(i) == already_processed_operands.end()) {
            operand_t *current_operand = &inst->getOp(i);

            // Only apply this analysis to registers
            if (current_operand->type == operand_t::REGISTER) {
              auto it_inst_dir = inst_operand_dir.find(current_operand->value.reg);

              if (it_inst_dir->second == reg_state::READ) {
                //////////////////////////////////////////////////////////////
                ////////////////////////// CASE READ /////////////////////////
                //////////////////////////////////////////////////////////////

                // Renamig = X, then we rename operand and start process again
                auto it_rename = registerRenaming.find(current_operand->value.reg);
                if (it_rename != registerRenaming.end()) {
                  SCMULATE_INFOMSG(5, "Register %s was previously renamed to %s", current_operand->value.reg.reg_name.c_str(), it_rename->second.reg_name.c_str());
                  current_operand->value.reg = it_rename->second;
                }

                auto it_used = used.find(current_operand->value.reg);
                if(it_used != used.end() && it_used->second == reg_state::WRITE) {
                  SCMULATE_INFOMSG(5, "Register %s is currently on WRITE state. Will subscribe but not sched", current_operand->value.reg.reg_name.c_str());
                } else if (it_used != used.end() && it_used->second == reg_state::READ) {
                  // Operand ready for execution
                  current_operand->full_empty = true;
                  SCMULATE_INFOMSG(5, "Register %s is currently on READ state. Subscribing and allowing sched", current_operand->value.reg.reg_name.c_str());
                } else {
                  // Insert it in used, and register to subscribers
                  used.insert(std::pair<decoded_reg_t, reg_state> (current_operand->value.reg, reg_state::READ));
                  // Operand ready for execution
                  current_operand->full_empty = true;
                  SCMULATE_INFOMSG(5, "Register %s was not found in the 'used' registers map. Marking as READ, subscribing, and allowing sched", current_operand->value.reg.reg_name.c_str());
                }
                // Attempt inserting if it does not exists already
                auto it_subs_insert = subscribers.insert(
                                      std::pair<decoded_reg_t, instruction_operand_ref_t >(
                                            current_operand->value.reg, instruction_operand_ref_t() ));
                it_subs_insert.first->second.push_back(instruction_operand_pair_t(inst_state, i));

              } else if (it_inst_dir->second == reg_state::WRITE) {
                //////////////////////////////////////////////////////////////
                ////////////////////////// CASE WRITE ////////////////////////
                //////////////////////////////////////////////////////////////

                auto it_used = used.find(current_operand->value.reg);
                decoded_reg_t original_reg = current_operand->value.reg;

                if(it_used != used.end()) {
                  // Register is in use. Let's rename it
                  decoded_reg_t newReg = getRenamedRegister(current_operand->value.reg);
                  // Check if renaming was successful.
                  if (newReg == current_operand->value.reg) {
                    SCMULATE_INFOMSG(5, "STRUCTURAL HAZZARD on operand %d. No new register was found for renaming. Leaving other operands for later SU iteration.", i);
                    hazzard_inst_state = inst_state;
                    inst_state->second = instruction_state::STALL;
                    return false;
                  }

                  auto it_rename = registerRenaming.insert(std::pair<decoded_reg_t, decoded_reg_t> (current_operand->value.reg, newReg));
                  renamedInUse.insert(newReg);
                  SCMULATE_INFOMSG(5, "Register %s is being 'used'. Renaming it to %s", current_operand->value.reg.reg_name.c_str(), newReg.reg_name.c_str());
                  if (!it_rename.second) {
                    SCMULATE_INFOMSG(5, "Register %s was already renamed to %s now it is changed to %s", current_operand->value.reg.reg_name.c_str(), it_rename.first->second.reg_name.c_str(), newReg.reg_name.c_str());
                    renamedInUse.erase(it_rename.first->second);
                    it_rename.first->second = newReg;
                  }
                  current_operand->value.reg = newReg;
                  // Insert it in used, and register to subscribers
                } else {
                  // Register not in used. Remove renaming for future references
                  auto it_rename = registerRenaming.find(current_operand->value.reg);
                  if (it_rename != registerRenaming.end()) {
                    renamedInUse.erase(it_rename->second);
                    registerRenaming.erase(it_rename);
                  }
                  SCMULATE_INFOMSG(5, "Register %s register was not found in the 'used' registers map. If renamed is set, we cleared it", current_operand->value.reg.reg_name.c_str());
                }

                // Search for all the other operands of the same, and apply changes to avoid conflicts. Only add once
                for (int other_op_num = 1; other_op_num <= MAX_NUM_OPERANDS; ++other_op_num) {
                  if (other_op_num != i && already_processed_operands.find(other_op_num) == already_processed_operands.end()) {
                    operand_t& other_op = inst->getOp(other_op_num);
                    if (other_op.type == operand_t::REGISTER && other_op.value.reg == original_reg) {
                      // Apply renaming if any then enable operand
                      other_op.value.reg = current_operand->value.reg;
                      other_op.full_empty = true;
                      already_processed_operands.insert(other_op_num);
                    }
                  }
                }
                SCMULATE_INFOMSG(5, "Register %s. Marking as WRITE and allowing sched", current_operand->value.reg.reg_name.c_str());
                used.insert(std::pair<decoded_reg_t, reg_state> (current_operand->value.reg, reg_state::WRITE));
                current_operand->full_empty = true;

              } else if (it_inst_dir->second == reg_state::READWRITE) {
                //////////////////////////////////////////////////////////////
                ////////////////////////// CASE READWRITE ////////////////////
                //////////////////////////////////////////////////////////////

                decoded_reg_t original_op_reg = current_operand->value.reg;
                decoded_reg_t original_renamed_reg = current_operand->value.reg;
                decoded_reg_t new_renamed_reg = current_operand->value.reg;

                // First check if operand was already renamed. Important for reading
                auto it_rename = registerRenaming.find(original_op_reg);
                if (it_rename != registerRenaming.end()) {
                  original_renamed_reg = it_rename->second;
                  SCMULATE_INFOMSG(5, "Register %s was previously renamed to %s", original_op_reg.reg_name.c_str(), original_renamed_reg.reg_name.c_str());
                }

                // Second check if the operand is currently being used. Important for writing
                auto it_used = used.find(original_op_reg);
                if(it_used != used.end()) {
                  // It is already being used, let's rename
                  new_renamed_reg = getRenamedRegister(original_op_reg);
                  // Check if renaming was successful.
                  if (new_renamed_reg == original_op_reg) {
                    SCMULATE_INFOMSG(5, "STRUCTURAL HAZZARD on operand %d. No new register was found for renaming. Leaving other operands for later SU iteration.", i);
                    hazzard_inst_state = inst_state;
                    inst_state->second = instruction_state::STALL;
                    return false;
                  }

                  auto it_rename = registerRenaming.insert(std::pair<decoded_reg_t, decoded_reg_t> (original_op_reg, new_renamed_reg));
                  renamedInUse.insert(new_renamed_reg);
                  SCMULATE_INFOMSG(5, "Register %s is being 'used'. Renaming it to %s", original_op_reg.reg_name.c_str(), new_renamed_reg.reg_name.c_str());
                  if (!it_rename.second) {
                    SCMULATE_INFOMSG(5, "Register %s was already renamed to %s now it is changed to %s", original_op_reg.reg_name.c_str(), original_renamed_reg.reg_name.c_str(), new_renamed_reg.reg_name.c_str());
                    renamedInUse.erase(it_rename.first->second);
                    it_rename.first->second = new_renamed_reg;
                  }
                } else {
                  if (it_rename != registerRenaming.end()) {
                    renamedInUse.erase(it_rename->second);
                    registerRenaming.erase(it_rename);
                  }
                  SCMULATE_INFOMSG(5, "Register %s was not found in the 'used' registers map. If it was previously renamed, we removed it", original_op_reg.reg_name.c_str());
                }

                // Third, check if current source (read) is available or not, to decide if broadcasting or subscription 
                // Stores if the operand is available for reading in variable available
                auto source_used = used.find(original_renamed_reg);
                bool available = source_used == used.end() || source_used->second == reg_state::READ;

                // Forth, if available make a copy into new register
                if (available && !(original_renamed_reg == new_renamed_reg)) {
                  SCMULATE_INFOMSG(5, "Copying register %s to register %s", original_renamed_reg.reg_name.c_str(), new_renamed_reg.reg_name.c_str());
                  std::memcpy(new_renamed_reg.reg_ptr, original_renamed_reg.reg_ptr, original_renamed_reg.reg_size_bytes);
                }

                // Fith. Apply renaming, and subscribe if not available
                for (int other_op_num = 1; other_op_num <= MAX_NUM_OPERANDS; ++other_op_num) {
                  if (already_processed_operands.find(other_op_num) == already_processed_operands.end()) {
                    operand_t& other_op = inst->getOp(other_op_num);
                    if (other_op.type == operand_t::REGISTER && other_op.value.reg == original_op_reg) {
                      // Apply register renaming
                      other_op.value.reg = new_renamed_reg;
                      // Decide scheduling or broadcasting
                      if (available) {
                        other_op.full_empty = true;
                        SCMULATE_INFOMSG(5, "Register %s op %d. Marking as WRITE and allowing sched", new_renamed_reg.reg_name.c_str(), other_op_num);
                      } else {
                        // Insert it in used, and register to broadcasters 
                        auto it_broadcast_insert = broadcasters.insert(
                                              std::pair<decoded_reg_t, instruction_operand_ref_t >(
                                                    original_renamed_reg, instruction_operand_ref_t() ));
                        it_broadcast_insert.first->second.push_back(instruction_operand_pair_t(inst_state, other_op_num));
                        SCMULATE_INFOMSG(5, "Register %s. Marking as WRITE. Do not allow to schedule until broadcast happens", new_renamed_reg.reg_name.c_str());
                      }
                      already_processed_operands.insert(other_op_num);
                    }
                  }
                }

                // Finally insert new_renamed_reg into used (remember that new_renamed_reg == original_op_reg if no renamed happened)
                used.insert(std::pair<decoded_reg_t, reg_state> (new_renamed_reg, reg_state::WRITE));
              }
            } else {
              // Operand is ready by default (IMMEDIATE or UNKNOWN)
              current_operand->full_empty = true;
            }
            already_processed_operands.insert(i);
          }
        }
        SCMULATE_INFOMSG(3, "Resulting Inst after analisys (%lu) %s", (unsigned long) inst, inst->getFullInstruction().c_str());

        // Of inst is a Codelet, we must update the values of the registers
        inst_state->first->updateCodeletParams();
        // We have finished processing this instruction, if it was a hazzard, it is not a hazzard anymore
        hazzard_inst_state = nullptr;

        // If an operand that requires memory instructions is not available, 
        // then we cannot reslve the memoryRange. We must stall the pipeline
        // If a memry region has another memory operation waiting for completioon, 
        // then we cannot reslve the memoryRange. We must stall the pipeline
        if(stallMemoryInstruction(inst)) {
          inst_state->second = instruction_state::STALL;
          SCMULATE_INFOMSG(5, "Stalling on instruction %s", inst->getFullInstruction().c_str());
          return false;
        }

        // once each operand dependency is analyzed, we make a decision if waiting, ready or stall. 
        // The type of instruction plays an important role here. 
        if (!isInstructionReady(inst)) {
          if (inst->getType() == instType::CONTROL_INST) {
            inst_state->second = instruction_state::STALL;
            SCMULATE_INFOMSG(5, "Stalling on instruction %s", inst->getFullInstruction().c_str());
          }
          return false;
        }

        // Instruction ready for execution 
        inst_state->second = instruction_state::READY;
        return true;
      }

      decoded_reg_t 
      ilp_OoO::getRenamedRegister(decoded_reg_t & otherReg) {
        static uint32_t curRegNum = 0;
        decoded_reg_t newReg = otherReg;
        uint32_t numReg4size = hidden_register_file->getNumRegForSize(newReg.reg_size_bytes);
        curRegNum = (curRegNum+1) % numReg4size;
        newReg.reg_number = curRegNum;
        uint32_t attempts = 0;

        // Iterate over the hidden register is found that is not being used
        do {
          newReg.reg_ptr = hidden_register_file->getNextRegister(newReg.reg_size_bytes, newReg.reg_number);
          attempts++;
        } while ((this->used.find(newReg) != this->used.end() || this->renamedInUse.find(newReg) != this->renamedInUse.end()) && attempts != numReg4size);
        if (attempts == numReg4size) {
          SCMULATE_INFOMSG(4, "When trying to rename, we could not find another register that was free out of %d", numReg4size);
          return otherReg;
        }
        newReg.reg_name = std::string("R_ren_") + newReg.reg_size + std::string("_") + std::to_string(newReg.reg_number);
        SCMULATE_INFOMSG(4, "Register %s mapped to %s with renaming", otherReg.reg_name.c_str(), newReg.reg_name.c_str());
        return newReg;
      }



      void 
      ilp_OoO::instructionFinished(instruction_state_pair * inst_state) {
        decoded_instruction_t * inst = inst_state->first;
        
        SCMULATE_INFOMSG(3, "Instruction (%lu) %s has finished. Starting clean up process", (unsigned long) inst,  inst->getFullInstruction().c_str() );
        // In memory instructions we need to remove range from memory
        if (inst->isMemoryInstruction()) {
          std::set <memory_location> * ranges = inst->getMemoryRange();
          memCtrl.removeRange( ranges );
        }
        
        getOperandsDirs(inst);
        // Depending on the operand direction, there is some book keeping that needs to happen, which 
        // also enables other instructions. See table 2 above
        for (auto it = inst_operand_dir.begin(); it != inst_operand_dir.end(); ++it) {

            // Check the operand's direction
            if (it->second == reg_state::READ) {
              auto it_used = used.find(it->first);
              // Check the state in the used map
              if (it_used->second == reg_state::READ) {
                auto subscribers_list_it = subscribers.find(it->first);
                if (subscribers_list_it != subscribers.end()) {
                  // Remove subscription
                  for (auto it_subs = subscribers_list_it->second.begin(); it_subs != subscribers_list_it->second.end();) {
                    if (it_subs->first->first == inst) {
                      it_subs = subscribers_list_it->second.erase(it_subs);
                      SCMULATE_INFOMSG(5, "Removing subscription in register %s, for instruction %s, operand %d", it->first.reg_name.c_str(), inst->getFullInstruction().c_str(), it_subs->second);
                    } else {
                      it_subs++;
                    }
                  }
                  // Move register to state NONE, if done
                  if (subscribers_list_it->second.size() == 0) {
                    subscribers.erase(subscribers_list_it);
                    used.erase(it_used);
                    SCMULATE_INFOMSG(5, "Subscriptions is empty. Move register %s to 'used' = NONE.", it->first.reg_name.c_str());
                  }
                } else {
                  SCMULATE_ERROR(0, "A read register should be subscribed to something");
                }
              } else {
                  SCMULATE_ERROR(0, "A read direction operand, should be 'used' marked as READ and it is %s", reg_state_str(it_used->second).c_str());
              }

            } else if (it->second == reg_state::WRITE || it->second == reg_state::READWRITE) {
              auto it_used = used.find(it->first);
              // Check the state in the used map
              if (it_used->second == reg_state::WRITE) {
                auto broadcaster_list_it = broadcasters.find(it->first);
                if (broadcaster_list_it != broadcasters.end()) {
                  // enable broadcasters operand and possibly instruction
                  for (auto it_broadcast = broadcaster_list_it->second.begin(); it_broadcast != broadcaster_list_it->second.end(); it_broadcast = broadcaster_list_it->second.erase(it_broadcast)) {
                    instruction_state_pair * other_inst_state_pair = it_broadcast->first;
                    // Broadcasting
                    // TODO: REMINDER: This may result in multiple copies of the same value. We must change it accordingly
                    std::memcpy(other_inst_state_pair->first->getOp(it_broadcast->second).value.reg.reg_ptr, it->first.reg_ptr, it->first.reg_size_bytes);
                    // Marking as ready
                    other_inst_state_pair->first->getOp(it_broadcast->second).full_empty = true;
                    SCMULATE_INFOMSG(5, "Enabling operand %d with register %s, for instruction %s", it_broadcast->second, it->first.reg_name.c_str(), other_inst_state_pair->first->getFullInstruction().c_str());
                    if (isInstructionReady(other_inst_state_pair->first)) {
                      if (!other_inst_state_pair->first->isMemoryInstruction() || other_inst_state_pair->second == instruction_state::WAITING || (other_inst_state_pair->second == instruction_state::STALL && !stallMemoryInstruction(other_inst_state_pair->first))) {
                        other_inst_state_pair->second = instruction_state::READY;
                        SCMULATE_INFOMSG(5, "Marking instruction %s as READY", other_inst_state_pair->first->getFullInstruction().c_str());
                      }
                    }
                  }
                  broadcasters.erase(broadcaster_list_it);
                }
                auto subscribers_list_it = subscribers.find(it->first);
                if (subscribers_list_it != subscribers.end()) {
                  // enable subscribed operand and possibly instruction
                  for (auto it_subs = subscribers_list_it->second.begin(); it_subs != subscribers_list_it->second.end(); it_subs++) {
                    instruction_state_pair * other_inst_state_pair = it_subs->first;
                    other_inst_state_pair->first->getOp(it_subs->second).full_empty = true;
                    SCMULATE_INFOMSG(5, "Enabling operand %d with register %s, for instruction %s", it_subs->second, it->first.reg_name.c_str(), other_inst_state_pair->first->getFullInstruction().c_str());
                    if (isInstructionReady(other_inst_state_pair->first)) {
                      if (!other_inst_state_pair->first->isMemoryInstruction() || other_inst_state_pair->second == instruction_state::WAITING || (other_inst_state_pair->second == instruction_state::STALL && !stallMemoryInstruction(other_inst_state_pair->first))) {
                        other_inst_state_pair->second = instruction_state::READY;
                        SCMULATE_INFOMSG(5, "Marking instruction %s as READY", other_inst_state_pair->first->getFullInstruction().c_str());
                      }
                    }
                  }
                  // Marking the register as read, ready to be consumed
                  it_used->second = reg_state::READ;
                } else {
                  used.erase(it_used);
                  SCMULATE_INFOMSG(5, "Register had no subscriptions. Move register %s to 'used' = NONE.", it->first.reg_name.c_str());
                }
              } else {
                  SCMULATE_ERROR(0, "A write direction operand, should be 'used' marked as WRITE and it is %s", reg_state_str(it_used->second).c_str());
              }
            } else {
              SCMULATE_ERROR(0, "What are you doing here???");
            }

          }

          // We are done with this instruction, we remove it from the reservation tables
          auto it_restable = reservationTable.find(inst_state);
          if (it_restable != reservationTable.end())
            reservationTable.erase(it_restable);
          else
            SCMULATE_ERROR(0, "Error, instruction %s was not in the reservation table", inst->getFullInstruction().c_str() );
      }

      bool 
      ilp_OoO::isInstructionReady(decoded_instruction_t * inst) {
        for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
          operand_t &thisOperand = inst->getOp(i);
          if(!thisOperand.full_empty) {
            return false;
          }
        }
        return true;
      }

      bool 
      ilp_OoO::stallMemoryInstruction(decoded_instruction_t * inst) {
        // Check if the instruction or its range overlap
        if (inst->isMemoryInstruction()) {
          // Check if an address operand is not ready
          for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
            operand_t &thisOperand = inst->getOp(i);
            if(!thisOperand.full_empty && inst->isOpAnAddress(i)) {
              return true;
              break;
            }
          }
          // Check if a memory region overlaps
          inst->calculateMemRanges();
          std::set <memory_location> * ranges = inst->getMemoryRange();
          if (memCtrl.itOverlaps( ranges )) {
            return true;
          }
          // The instruction is ready to schedule, let's mark the ranges as busy
          memCtrl.addRange( ranges );
        }
        return false;

      }

      void 
      ilp_OoO::getOperandsDirs (decoded_instruction_t * inst) {
        inst_operand_dir.clear();
        for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
          if (inst->getOp(i).type == operand_t::REGISTER) {
            // Attempt to insert the register, if it is already there, the insert will return a pair with the iterator to the 
            // already inserted value.
            auto it_insert = inst_operand_dir.insert(std::pair<decoded_reg_t, reg_state>(inst->getOp(i).value.reg, reg_state::NONE));
            reg_state &cur_state = it_insert.first->second;

            // We need to set the value of reg_state to read, write or readwrite
            if ( (inst->getOp(i).read && inst->getOp(i).write ) ||
                 (inst->getOp(i).read && cur_state == reg_state::WRITE ) ||
                 (inst->getOp(i).write && cur_state == reg_state::READ) ) 
              cur_state = reg_state::READWRITE;
            else if (inst->getOp(i).write && cur_state == reg_state::NONE)
              cur_state = reg_state::WRITE;
            else if (inst->getOp(i).read && cur_state == reg_state::NONE)
              cur_state = reg_state::READ;
            
            SCMULATE_INFOMSG(5, "In instruction %s Register %s set as %s", inst->getFullInstruction().c_str() , inst->getOp(i).value.reg.reg_name.c_str(), reg_state_str(cur_state).c_str());
        }
      }
    }

}