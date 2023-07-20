#include "codelet_duplicator.hpp"
#include "fetch_decode.hpp"

#define RESILIENCY_ERROR                                                       \
  {                                                                            \
    printf("RESILIENCY_ERROR\n");                                              \
    exit(999);                                                                 \
  }
namespace scm {

class fetch_decode_module;

decoded_reg_t
dupl_controller_module::getRenamedRegister(decoded_reg_t &otherReg) {
  // static uint32_t curRegNum = 0;
  decoded_reg_t newReg = otherReg;
  uint32_t numReg4size =
      hidden_reg_file_m.getNumRegForSize(newReg.reg_size_bytes);
  // curRegNum = (curRegNum + 1) % numReg4size;
  // newReg.reg_number = curRegNum;
  uint32_t attempts = 0;

  // Iterate over the hidden register is found that is not being used
  do {
    newReg.reg_ptr = hidden_reg_file_m.getNextRegister(newReg.reg_size_bytes,
                                                       newReg.reg_number);
    SCMULATE_INFOMSG(6, "Trying to rename register %d to %d",
                     otherReg.reg_number, newReg.reg_number);
    attempts++;
  } while (
      (this->renamedInUse.find(newReg.reg_ptr) != this->renamedInUse.end()) &&
      attempts != numReg4size);
  if (attempts == numReg4size) {
    SCMULATE_INFOMSG(4,
                     "When trying to rename, we could not find another "
                     "register that was free out of %d",
                     numReg4size);
    return otherReg;
  }
  newReg.reg_name = std::string("R_ren_") + newReg.reg_size + std::string("_") +
                    std::to_string(newReg.reg_number);
  SCMULATE_INFOMSG(4, "Register %s mapped to %s with renaming",
                   otherReg.reg_name.c_str(), newReg.reg_name.c_str());
  return newReg;
}

decoded_instruction_t *dupl_controller_module::duplicateCodeletInstruction(
    instruction_state_pair *original) {
  // Create a new instruction with the same operation
  // Then change the instruction
  decoded_instruction_t *new_inst = new decoded_instruction_t(*original->first);

  SCMULATE_INFOMSG(5, "Duplicating instruction %s at (0x%lX)",
                   original->first->getFullInstruction().c_str(),
                   (unsigned long)original);

  // Change the instruction name
  new_inst->setInstruction(std::string("DUP_") +
                           original->first->getInstruction());

  // Iterate over the operands, search for write and readwrite
  std::unordered_map<decoded_reg_t, reg_state> *inst_operand_dir =
      new_inst->getOperandsDirs();

  bool wasRenamed = false;

  for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
    operand_t *current_operand = &original->first->getOp(i);
    operand_t *new_operands = &new_inst->getOp(i);
    if (current_operand->type == operand_t::REGISTER) {
      auto it_inst_dir = inst_operand_dir->find(current_operand->value.reg);

      if (it_inst_dir->second == reg_state::WRITE ||
          it_inst_dir->second == reg_state::READWRITE) {
        SCMULATE_INFOMSG(5,
                         "Found operand %d in instruction %s with WRITE or "
                         "READWRITE with register %s",
                         i, original->first->getFullInstruction().c_str(),
                         current_operand->value.reg.reg_name.c_str());
        decoded_reg_t newReg = getRenamedRegister(current_operand->value.reg);
        // Check if renaming was successful.
        SCMULATE_ERROR_IF(
            0, newReg == current_operand->value.reg,
            "Trying to rename operand %d during codelet duplication. No "
            "new register was found "
            "for renaming. Leaving other operands for later SU iteration.",
            i);
        SCMULATE_INFOMSG(5, "Inserting %s (0x%lX) into renamedInUse",
                         newReg.reg_name.c_str(),
                         (unsigned long)newReg.reg_ptr);
        renamedInUse.insert(newReg.reg_ptr);
        new_operands->value.reg = newReg;
        SCMULATE_INFOMSG(5, "Renaming operand %d from %s to %s", i,
                         current_operand->value.reg.reg_name.c_str(),
                         newReg.reg_name.c_str());
        SCMULATE_INFOMSG(5, "Renamed pointers have addresses %p and %p",
                         current_operand->value.reg.reg_ptr, newReg.reg_ptr);
        if (it_inst_dir->second == reg_state::READWRITE) {
          SCMULATE_INFOMSG(
              5, "Operand %d in instruction %s is READWRITE with register %s",
              i, original->first->getFullInstruction().c_str(),
              current_operand->value.reg.reg_name.c_str());
          // Check if it does not exist in the originals list.
          // If not, get yet another register and store the original value
          // there.
          auto it_orig = originalVals.find(current_operand->value.reg);
          if (it_orig == originalVals.end()) {
            decoded_reg_t origReg =
                getRenamedRegister(current_operand->value.reg);
            SCMULATE_ERROR_IF(
                0, origReg == current_operand->value.reg,
                "Trying to rename operand %d during codelet duplication. No "
                "new register was found "
                "for renaming. Leaving other operands for later SU "
                "iteration.",
                i);
            SCMULATE_INFOMSG(5, "Renaming operand %d from %s to %s", i,
                             current_operand->value.reg.reg_name.c_str(),
                             origReg.reg_name.c_str());
            SCMULATE_INFOMSG(5, "Inserting %s (0x%lX) into originalVals",
                             current_operand->value.reg.reg_name.c_str(),
                             (unsigned long)current_operand->value.reg.reg_ptr);
            originalVals[current_operand->value.reg] = origReg;
            // Copy the value from the original register to the new one
            std::memcpy(origReg.reg_ptr, current_operand->value.reg.reg_ptr,
                        origReg.reg_size_bytes);
            SCMULATE_INFOMSG(5, "Inserting %s (0x%lX) into renamedInUse",
                             origReg.reg_name.c_str(),
                             (unsigned long)origReg.reg_ptr);
            renamedInUse.insert(origReg.reg_ptr);
          } else {
            SCMULATE_INFOMSG(
                5, "Copying original content of operand %d from %s to %s", i,
                current_operand->value.reg.reg_name.c_str(),
                it_orig->second.reg_name.c_str());
            // Copy the value from the original register to the new one
            std::memcpy(newReg.reg_ptr, it_orig->second.reg_ptr,
                        newReg.reg_size_bytes);
          }
        }

        wasRenamed = true;
      }
    }
  }
  if (wasRenamed) {
    // Of inst is a Codelet, we must update the values of the registers
    new_inst->updateCodeletParams();
    new_inst->calculateOperandsDirs();
  }

  return new_inst;
}

void dupl_controller_module::doReschedulingOfReadyCodelets(
    instruction_state_pair *original) {
  dupCodeletStates *allCopies = inst_buff_m->getDuplicated(original);

  for (auto copy : *allCopies) {
    if (copy->second == instruction_state::READY) {
      SCMULATE_INFOMSG(5, "Trying to schedule new copy of %s",
                       copy->first->getFullInstruction().c_str());
      copy->second = instruction_state::EXECUTING;
      if (!fetch_decode_m->attemptAssignExecuteInstruction(copy)) {
        SCMULATE_INFOMSG(5,
                         "Could not schedule new copy of %s. Waiting for "
                         "later.",
                         copy->first->getFullInstruction().c_str());
        copy->second = instruction_state::READY;
      }
    }
  }
}

void dupl_controller_module::createDuplicatedCodelet(
    instruction_state_pair *inst, int copies) {
  // Duplicate codelet: We create a new instruction pair that
  // contains a new duplicated decoded instruction, where write and
  // readwrite operands are renamed to a hidden operand. We assign
  // instruction_state::EXECUTING
  for (int i = 0; i < copies; i++) {
    auto duplicatedCodelet = duplicateCodeletInstruction(inst);
    instruction_state_pair *newPair = new instruction_state_pair(
        duplicatedCodelet, instruction_state::EXECUTING);
    inst_buff_m->insertDuplicatedCodelets(inst, newPair);
    SCMULATE_INFOMSG(5, "Trying to schedule new copy of %s",
                     duplicatedCodelet->getFullInstruction().c_str());
    if (!fetch_decode_m->attemptAssignExecuteInstruction(newPair)) {
      SCMULATE_INFOMSG(5,
                       "Could not schedule new copy of %s. Waiting for later.",
                       duplicatedCodelet->getFullInstruction().c_str());
      newPair->second = instruction_state::READY;
    }
  }
}

bool dupl_controller_module::compareTwoCodelets(
    instruction_state_pair *original, instruction_state_pair *duplicate) {

  decoded_instruction_t *inst = original->first;
  std::unordered_map<decoded_reg_t, reg_state> *inst_operand_dir =
      inst->getOperandsDirs();

  SCMULATE_INFOMSG(5, "Comparing instruction %s",
                   inst->getFullInstruction().c_str());
#if REGISTER_INJECTION_MODE == 1
  for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
    operand_t *current_operand = &inst->getOp(i);
    if (current_operand->type == operand_t::REGISTER) {
      auto it_inst_dir = inst_operand_dir->find(current_operand->value.reg);

      if (it_inst_dir->second == reg_state::WRITE ||
          it_inst_dir->second == reg_state::READWRITE) {
        decoded_reg_t orig = current_operand->value.reg;
        decoded_instruction_t *dup_inst = duplicate->first;
        operand_t *target_compare_operand = &dup_inst->getOp(i);
        decoded_reg_t dup = target_compare_operand->value.reg;
        if (!(orig & dup)) {
          SCMULATE_INFOMSG(5, "Instruction %s differs from %s in operand %d",
                           inst->getFullInstruction().c_str(),
                           dup_inst->getFullInstruction().c_str(), i);
          return false;
        }
      }
    }
  }
#else
  if (original->first->getItFailed() || duplicate->first->getItFailed()) {
    SCMULATE_INFOMSG(5,
                     "Instruction %s or %s failed. Therefore comparison fails",
                     inst->getFullInstruction().c_str(),
                     duplicate->first->getFullInstruction().c_str());
    return false;
  }
#endif
  SCMULATE_INFOMSG(5, "Instruction %s is equal to %s in all operands",
                   inst->getFullInstruction().c_str(),
                   duplicate->first->getFullInstruction().c_str());
  return true;
}

void dupl_controller_module::overrideOriginalOperands(
    instruction_state_pair *original, instruction_state_pair *duplicate) {
  decoded_instruction_t *inst = original->first;
  std::unordered_map<decoded_reg_t, reg_state> *inst_operand_dir =
      inst->getOperandsDirs();

  SCMULATE_INFOMSG(
      5, "Overriding operands of instruction %s because the original failed",
      inst->getFullInstruction().c_str());

  for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
    operand_t *current_operand = &inst->getOp(i);
    if (current_operand->type == operand_t::REGISTER) {
      auto it_inst_dir = inst_operand_dir->find(current_operand->value.reg);

      if (it_inst_dir->second == reg_state::WRITE ||
          it_inst_dir->second == reg_state::READWRITE) {
        decoded_reg_t orig = current_operand->value.reg;
        decoded_instruction_t *dup_inst = duplicate->first;
        operand_t *source_operand_to_copy = &dup_inst->getOp(i);
        decoded_reg_t source_operand = source_operand_to_copy->value.reg;
        // Perform a copy of the right value
        std::memcpy(source_operand.reg_ptr, orig.reg_ptr,
                    source_operand.reg_size_bytes);
      }
    }
  }
}

dupl_controller_module::dupl_controller_module(
    DUPL_MODES m, fetch_decode_module *fd, instructions_buffer_module *instBuff)
    : inst_buff_m(instBuff), fetch_decode_m(fd) {

  // Check if SCM_DUPL_MODE env variable is set
  char *dupl_mode_env = std::getenv("SCM_DUPL_MODE");
  if (dupl_mode_env != NULL) {
    std::string dupl_mode_str(dupl_mode_env);
    if (dupl_mode_str == "NO_DUPLICATION") {
      mode = DUPL_MODES::NO_DUPLICATION;
    } else if (dupl_mode_str == "TWO_OUT_OF_THREE") {
      mode = DUPL_MODES::TWO_OUT_OF_THREE;
    } else if (dupl_mode_str == "THREE_OUT_OF_FIVE") {
      mode = DUPL_MODES::THREE_OUT_OF_FIVE;
    } else if (dupl_mode_str == "ADAPTIVE_DUPLICATION") {
      mode = DUPL_MODES::ADAPTIVE_DUPLICATION;
    } else {
      SCMULATE_ERROR(0, "Invalid duplication mode: %s", dupl_mode_str.c_str());
    }
  } else {
    mode = m;
  }

  SCMULATE_INFOMSG(2, "Initializing codelet duplicator with mode: %s",
                   DUPL_MODES_STR[m]);
}

dupl_controller_module::dupl_controller_module(
    fetch_decode_module *fd, instructions_buffer_module *instBuff)
    : dupl_controller_module(DUPL_MODES::NO_DUPLICATION, fd, instBuff) {}

void dupl_controller_module::duplicateCodelet(instruction_state_pair *inst) {
  if (inst->first->getType() != scm::instType::EXECUTE_INST ||
      inst->first->getExecCodelet()->isMemoryCodelet())
    return;
  if (mode == DUPL_MODES::NO_DUPLICATION)
    return;
  else if (mode == DUPL_MODES::TWO_OUT_OF_THREE) {
    createDuplicatedCodelet(inst, 2);
  } else if (mode == DUPL_MODES::THREE_OUT_OF_FIVE) {
    createDuplicatedCodelet(inst, 4);
  } else if (mode == DUPL_MODES::ADAPTIVE_DUPLICATION) {
    createDuplicatedCodelet(inst);
  }
}

bool dupl_controller_module::compareCodelets(instruction_state_pair *inst) {
  if (inst->first->getType() != scm::instType::EXECUTE_INST ||
      inst->first->getExecCodelet()->isMemoryCodelet())
    return true;

  if (mode == DUPL_MODES::NO_DUPLICATION) {
    if (inst->first->getItFailed()) {
      RESILIENCY_ERROR;
    } else {
      return true;
    }
  }
  // Attempt to schedule those codelets that have not been scheduled
  doReschedulingOfReadyCodelets(inst);

  // checkIfReady will check all the copies of the current instruction
  // otherwise it will wait for the other copies to finish
  if (inst_buff_m->checkIfReady(inst)) {
    SCMULATE_INFOMSG(5,
                     "Instruction %s at (0x%lX) is ready for comparison. All "
                     "copies have finished",
                     inst->first->getFullInstruction().c_str(),
                     (unsigned long)inst);
    // compare
    dupCodeletStates *allCopies = inst_buff_m->getDuplicated(inst);

#if REGISTER_INJECTION_MODE == 0
    // Print all the itFailed variables
    for (auto copy : *allCopies) {
      SCMULATE_INFOMSG(5, "Instruction %s at (0x%lX) has itFailed %s",
                       copy->first->getFullInstruction().c_str(),
                       (unsigned long)copy,
                       copy->first->getItFailed() ? "true" : "false");
    }
    SCMULATE_INFOMSG(5, "Instruction %s at (0x%lX) has itFailed %s",
                     inst->first->getFullInstruction().c_str(),
                     (unsigned long)inst,
                     inst->first->getItFailed() ? "true" : "false");

#endif

    if (mode == DUPL_MODES::TWO_OUT_OF_THREE) {
      SCMULATE_ERROR_IF(0, allCopies->size() != 2,
                        "Error: Two out of three mode requires two copies");
      if (compareTwoCodelets(inst, allCopies->at(0)) ||
          compareTwoCodelets(inst, allCopies->at(1)))
        return true;

      // The duplicates are the right values, override the original
      if (compareTwoCodelets(allCopies->at(0), allCopies->at(1))) {
        overrideOriginalOperands(inst, allCopies->at(0));
        return true;
      }
      RESILIENCY_ERROR;
      return false;
    } else if (mode == DUPL_MODES::THREE_OUT_OF_FIVE) {
      SCMULATE_ERROR_IF(0, allCopies->size() != 4,
                        "Error: Three out of five mode requires four copies");
      // Make a copy of all codelets and add the original
      dupCodeletStates allCopiesAndOriginal;
      allCopiesAndOriginal.push_back(inst);
      allCopiesAndOriginal.insert(allCopiesAndOriginal.end(),
                                  allCopies->begin(), allCopies->end());
      auto it = allCopiesAndOriginal.begin();
      for (; it != allCopiesAndOriginal.end(); ++it) {
        auto it2 = it + 1;
        for (; it2 != allCopiesAndOriginal.end(); ++it2)
          if (compareTwoCodelets(*it, *it2)) {
            auto it3 = it2 + 1;
            for (; it3 != allCopiesAndOriginal.end(); ++it3)
              if (compareTwoCodelets(*it, *it3)) {
                if (*it != inst && *it2 != inst && *it3 != inst)
                  overrideOriginalOperands(inst, *it);
                return true;
              }
          }
      }

      RESILIENCY_ERROR;
      return false;
    } else if (mode == DUPL_MODES::ADAPTIVE_DUPLICATION) {
      // Make a copy of all codelets and add the original
      dupCodeletStates allCopiesAndOriginal;
      allCopiesAndOriginal.push_back(inst);
      allCopiesAndOriginal.insert(allCopiesAndOriginal.end(),
                                  allCopies->begin(), allCopies->end());

      // Map of similar codelets
      std::unordered_map<instruction_state_pair *,
                         std::vector<instruction_state_pair *>>
          similarCodelets;
      std::vector<bool> alreadyAdded;
      alreadyAdded.resize(allCopiesAndOriginal.size(), false);

      // Check for consensus
      auto it = allCopiesAndOriginal.begin();
      // Iterate over all the codelets, comparing them to all the others
      for (; it != allCopiesAndOriginal.end(); ++it) {
        // If we added it already, it means that this has already found
        // a match before. Then we skip this codelet
        int it_index = std::distance(allCopiesAndOriginal.begin(), it);
        if (alreadyAdded[it_index])
          continue;
        // No previous match, so we create a new vector for this codelet
        std::vector<instruction_state_pair *> v;
        v.push_back(*it);
        similarCodelets.insert({*it, v});
        // Compare this codelet to all the others
        auto it2 = it + 1;
        for (; it2 != allCopiesAndOriginal.end(); ++it2) {
          // If the other codelet was already added, skip it
          int it2_index = std::distance(allCopiesAndOriginal.begin(), it2);
          if (alreadyAdded[it2_index])
            continue;
          // Compare the two codelets
          if (compareTwoCodelets(*it, *it2)) {
            // If they are similar, add them to the map for *it
            similarCodelets[*it].push_back(*it2);
            // Mark the other codelet as added to a map
            alreadyAdded[it2_index] = true;
            // Check if we have reached consensus. Concentus is reached
            // if the number of similar codelets is greater than half of
            // the number of codelets that have been duplicated so far
            if (similarCodelets[*it].size() > allCopiesAndOriginal.size() / 2) {
              // We have reached consensus, override the original
              // codelet operands if it is not within the consensus
              if (std::find(similarCodelets[*it].begin(),
                            similarCodelets[*it].end(),
                            inst) == similarCodelets[*it].end()) {
                overrideOriginalOperands(inst, *it);
              }
              return true;
            }
          }
        }
      }
      // We have not reached consensus, so we need to create a new copy
      // as long as we have not reached the maximum number of copies
      if (allCopiesAndOriginal.size() < MAX_DUPLICATED_CODELETS) {
        createDuplicatedCodelet(inst);
        return false;
      } else {
        RESILIENCY_ERROR;
        return false;
      }
    }
  }
  SCMULATE_INFOMSG(10, "Waiting for all the duplicates to be ready")
  // wait
  return false;
}

void dupl_controller_module::cleanupDuplication(instruction_state_pair *inst) {
  if (inst->first->getType() != scm::instType::EXECUTE_INST ||
      inst->first->getExecCodelet()->isMemoryCodelet())
    return;
  if (mode == DUPL_MODES::NO_DUPLICATION)
    return;
  dupCodeletStates *allCopies = inst_buff_m->getDuplicated(inst);
  dupCodeletStates allCopiesAndOriginal;
  allCopiesAndOriginal.push_back(inst);
  allCopiesAndOriginal.insert(allCopiesAndOriginal.end(), allCopies->begin(),
                              allCopies->end());
  // Iterate and if operand found, remove it
  for (auto copy : allCopiesAndOriginal) {
    for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
      operand_t *current_operand = &copy->first->getOp(i);
      if (current_operand->type == operand_t::REGISTER) {
        if (this->renamedInUse.find(current_operand->value.reg.reg_ptr) !=
            this->renamedInUse.end()) {
          SCMULATE_INFOMSG(6,
                           "Removing renamed register %s with address (0x%lX)",
                           current_operand->value.reg.reg_name.c_str(),
                           (unsigned long)current_operand->value.reg.reg_ptr);
          this->renamedInUse.erase(current_operand->value.reg.reg_ptr);
        }
        auto it = originalVals.find(current_operand->value.reg);
        if (it != originalVals.end()) {
          SCMULATE_INFOMSG(
              6, "Removing renamed register %s with address (0x%lX)",
              it->second.reg_name.c_str(), (unsigned long)it->second.reg_ptr);
          this->renamedInUse.erase(it->second.reg_ptr);
          SCMULATE_INFOMSG(6, "Removing OriginalVals register %s",
                           current_operand->value.reg.reg_name.c_str());

          originalVals.erase(current_operand->value.reg);
        }
      }
    }
  }
  // Remove the duplicates from the buffer
  inst_buff_m->removeDuplicates(inst);
}

dupl_controller_module::~dupl_controller_module() {}

} // namespace scm