#ifndef __ILP_CONTROLLER__
#define __ILP_CONTROLLER__

/** \brief Instruction Level Parallelism Controller 
 *
 * This file contains all the necessary definitions and book keeping to 
 * know if an instruction can be scheduled or not. It contains the needed tables that
 * keep track of data dependencies and others. 
 */

#include "SCMUlate_tools.hpp"
#include "system_config.hpp"
#include "instructions.hpp"
#include "instruction_mem.hpp"
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <queue>
#include <limits>

/**
 * Potential hazards:
 * 
 * Read after read: Not a problem
 * COD R1, R2, R3 (W, R, R)
 * COD R4, R2, R5 (W, R, R) // R2 is read again.
 * Here we don't need to do anything since R2 will not be changed. 
 * The two operations can be scheduled at the same time
 * 
 * Write after read: You cannot write because it is needed for a read.
 * COD R1, R2, R3 (W, R, R)
 * COD R2, R4, R5 (W, R, R) // R2 is going to be overwritten
 * We have to wait until R2 is consumed. However, in the origina tomasulo's
 * algorithm the value of R2 is copied in the reservation table, freeing the 
 * register R2 to be written. In our implementation we need to halt the second
 * instruction, marking the register busy, since we do not allow yet to have a 
 * copy of the operand in the reservation table. This would be costly as we are 
 * relying on whole copies to the register which go all the way to DRAM. 
 * Instead a register renaming scheme would solve this issue
 * 
 * 
 * Write after write: You cannot write until the first write is done.
 * COD R1, R2, R3 (W, R, R)
 * COD R1, R4, R5 (W, R, R) // R1 is going to be overwritten
 * We have to wait until the first instruction is executed.
 * Similar things happen in the write after read case. 
 * 
 * Read after write: You cannot read until the write happens. 
 * COD R1, R2, R3 (W, R, R)
 * COD R4, R1, R5 (W, R, R) // R1 needs to be produced first
 * This is the real dependency and we will have to wait until the first one is 
 * finished. 
 * 
 * In general the easy scheduling algorithm will do this:
 * 1. Mark all the registers as busy. For mem operations check memory overlapping. 
 * 2. If all the registers that are read operands are not busy then we start executing the operation.
 * 3. If we are missing a register, we stall the execution.
 * 
 * Memory operations are a little harder because we are required to check if the destination address is being op on
 * The next step is to implement register renaming which will eliminate false dependencies
 * And we should only stall if it is a true dependency.
 * 
 * After this, attempting to implement a version based on the tomasulo's algorithm
 * would be ideal. The trick is to somehow build the dep tree during execution. 
 * A big question is what to do with loops. 
 */

namespace scm {
  
  struct register_reservation
  {
    std::string reg_name;
    std::uint_fast16_t reg_direction;
    register_reservation() : reg_name(""), reg_direction(OP_IO::NO_RD_WR) {}
    register_reservation(const register_reservation &other) : reg_name(other.reg_name), reg_direction(other.reg_direction) {}
    register_reservation(std::string name, std::uint_fast16_t mask) : reg_name(name), reg_direction(mask) {}
    inline bool operator<(const register_reservation &other) const
    {
      return this->reg_name < other.reg_name;
    }
  };

  class memory_queue_controller {
    private:
      // Ranges are inclusive on begining and exclusive on end
      std::set<memory_location> ranges;
    public:
      memory_queue_controller() { };
      uint32_t inline numberOfRanges () { return ranges.size(); }
      void inline addRange(memory_location& curLocation) {
        SCMULATE_INFOMSG(4, "Adding range [%lu, %lu, %lu]",(unsigned long)curLocation.memoryAddress, (unsigned long)curLocation.memoryAddress+curLocation.size, (unsigned long)curLocation.size );
        this->ranges.insert(curLocation);
      }
      void inline removeRange(memory_location& curLocation) {
        SCMULATE_INFOMSG(4, "Removing range [%lu, %lu, %lu]",(unsigned long)curLocation.memoryAddress, (unsigned long)curLocation.memoryAddress+curLocation.size, (unsigned long)curLocation.size );
        if (this->ranges.find(curLocation) != this->ranges.end()) {
          this->ranges.erase(curLocation);
        } else {
          SCMULATE_ERROR(0, "The range [%lu, %lu, %lu], does not exist in the set of ranges.", (unsigned long)curLocation.memoryAddress, (unsigned long)curLocation.memoryAddress+curLocation.size, (unsigned long)curLocation.size)
        }
      }
      bool inline itOverlaps(memory_location& curLocation) {
        // The start of the curLocation cannot be in between a beginning and end of the range
        // The end of the curLocation cannot be between a beginning and end
        for (auto itRange = ranges.begin(); itRange != ranges.end(); itRange++) {
          if (itRange->memoryAddress <= curLocation.memoryAddress && itRange->upperLimit() > curLocation.memoryAddress) {
            return true;
          }
          if (itRange->memoryAddress < curLocation.upperLimit() && itRange->upperLimit() > curLocation.upperLimit()) {
            return true;
          }
          if (curLocation.memoryAddress <= itRange->memoryAddress && curLocation.upperLimit() >= itRange->upperLimit() ) {
            return true;
          }
        }
        return false;
      }
  };


  /**
   * @brief Superscalar execution mode
   * 
   * This mode supports overlapping of instructions that have no dependencies or that they are RAR dependencies
   * This mode stalls whenever a WAR WAW RAW dependencies are encountered. 
   * 
   */
  class ilp_superscalar {
    private:
      memory_queue_controller memCtrl;
      std::set<register_reservation> busyRegisters;
      //std::queue<decoded_instruction_t> reservationTable;
      std::set<memory_location> memoryLocations;
    public:
      ilp_superscalar() { }
      /** \brief check if instruction can be scheduled 
      * Returns true if the instruction could be scheduled according to
      * the current detected hazards. If it is possible to schedule it, then
      * it will add the list of detected hazards to the tracking tables
      */
      bool inline checkMarkInstructionToSched(instruction_state_pair * inst_state) {
        decoded_instruction_t * inst = (inst_state->first);
        if (inst->getType() == instType::COMMIT && (busyRegisters.size() != 0 || memCtrl.numberOfRanges() != 0)) {
          inst_state->second = instruction_state::STALL;
          return false;
        }

        if (inst->getOp1().type == operand_t::REGISTER && hazardExist(inst->getOp1().value.reg.reg_name, (inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR)))) {
          inst_state->second = instruction_state::STALL;
          return false;
        }
        if (inst->getOp2().type == operand_t::REGISTER && hazardExist(inst->getOp2().value.reg.reg_name, (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR))>>2)) {
          inst_state->second = instruction_state::STALL;
          return false;
        }
        if (inst->getOp3().type == operand_t::REGISTER && hazardExist(inst->getOp3().value.reg.reg_name, (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR))>>4)) {
          inst_state->second = instruction_state::STALL;
          return false;
        }

        // In memory instructions we need to figure out if there is a hazard in the memory
        if (inst->isMemoryInstruction()) {
          // Check all the ranges
          std::vector <memory_location> ranges = inst->getMemoryRange();
          for (auto it = ranges.begin(); it != ranges.end(); ++it) {
            if (memCtrl.itOverlaps( *it )) {
              inst_state->second = instruction_state::STALL;
              return false;
            }
          }

          // The instruction is ready to schedule, let's mark the ranges as busy
          for (auto it = ranges.begin(); it != ranges.end(); ++it)
            memCtrl.addRange( *it );
        } 
        // Mark the registers
        if (inst->getOp1().type == operand_t::REGISTER) {
          uint_fast16_t io = inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR);
          SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp1().value.reg.reg_name.c_str(), io);
          register_reservation reserv(inst->getOp1().value.reg.reg_name, io);
          busyRegisters.insert(reserv);
        }
        if (inst->getOp2().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR)) >> 2;
          SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp2().value.reg.reg_name.c_str(), io);
          register_reservation reserv(inst->getOp2().value.reg.reg_name, io);
          busyRegisters.insert(reserv);          
          }
        if (inst->getOp3().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR)) >> 4;
          SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp3().value.reg.reg_name.c_str(), io);
          register_reservation reserv(inst->getOp3().value.reg.reg_name, io);
          busyRegisters.insert(reserv);          
          }
        inst_state->second = instruction_state::READY;
        return true;
      }

      void inline instructionFinished(instruction_state_pair * inst_state) {
        decoded_instruction_t * inst = inst_state->first;
        // In memory instructions we need to figure out if there is a hazard in the memory
        if (inst->isMemoryInstruction()) {
          std::vector <memory_location> ranges = inst->getMemoryRange();
          for (auto it = ranges.begin(); it != ranges.end(); ++it)
            memCtrl.removeRange( *it );
        } 

        if (inst->getOp1().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR));
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX", inst->getOp1().value.reg.reg_name.c_str(), io );
          eraseReg(inst->getOp1().value.reg.reg_name, io);
        }
        if (inst->getOp2().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR)) >> 2;
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX",inst->getOp2().value.reg.reg_name.c_str(), io );
          eraseReg(inst->getOp2().value.reg.reg_name, io);
        }
        if (inst->getOp3().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR)) >> 4;
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX",inst->getOp3().value.reg.reg_name.c_str(), io);
          eraseReg(inst->getOp3().value.reg.reg_name, io);
        }
        SCMULATE_INFOMSG(5, "The number of busy regs is %lu", this->busyRegisters.size());
      }

      bool inline hazardExist(std::string& regName, uint_fast16_t io_dir) { 
        auto foundReg = busyRegisters.find(register_reservation(regName, io_dir));
        bool isFound = busyRegisters.find(register_reservation(regName, io_dir)) != busyRegisters.end();
        SCMULATE_INFOMSG_IF(5, isFound, "Hazard detected");
        if (!isFound) return false;
        if ((foundReg->reg_direction & OP_IO::OP1_WR) | (io_dir & OP_IO::OP1_WR)) return true; // WAW or RAW or WAR
        return false;
      }
      void inline eraseReg(std::string& regName, uint_fast16_t io_dir) { 
       busyRegisters.erase(register_reservation(regName, io_dir));
      }
  };


  /**
   * @brief Out of Order execution mode
   * 
   * This mode supports overlapping of false dependencies: RAR, WAR, and WAW. It allows register renaming as well as out of
   * order execution between instructions. Commit is guaranteed through in-order memory operations to the same memory location
   * 
   * When an instruction is new, a process to determine its scheduling is sumarized in the following table:
   *
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | register is | used is       | rename is   | result                     | schedule?              |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | Read        | Any or None   | Set to X    | operand = X                | Continue process       |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | Read        | None          | None        | Used = Read                | Allow Scheduling       |
   * |             |               |             | subscribe += inst, operand |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | Read        | Read          | None        | subscribe += inst, operand | Allow Scheduling       |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | Read        | write         | none        | subscribe += inst, operand | Don't allow scheduling |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | Write       | None          | Any or None | Used = write               | Allow Scheduling       |
   * |             |               |             | Rename = None              |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | Write       | Read or Write | Any or None | rename = other             | Allow Scheduling       |
   * |             |               |             | operand = other            |                        |
   * |             |               |             | used = write               |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * 
   * 
   * ////// FIX THIS BOTTOM TABLE. 
   * | ReadWrite   | Any or None   | Set to X    | renamed = x                | Apply table to renamed |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | ReadWrite   | None          | None        | Used = write               | Allow Scheduling       |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | ReadWrite   | Read          | None        | Rename = other             | Allow Scheduling       |
   * |             |               |             | Copy Reg to other          |                        |
   * |             |               |             | operand = other            |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | ReadWrite   | Write         | None        | Rename = other             | Don't allow            |
   * |             |               |             | other used = write         |                        |
   * |             |               |             | broadcast += other         |                        |
   * |             |               |             | operand = other            |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * 
   * The state of a register is determined by three values: 
   *  -> Used:         This specifies if the register is busy. It may be read, write or none. When writting, there
   *                   is only a single instruction "owning" the register. When read, there may be multiple 
   *                   instructions reading from it. When none, there are no owners and it can be used.
   *  -> Rename:       Determines that any further reading references to that register must refer to another register.
   *                   If this value is not set (nullptr or "") there is no renaming. Depending on the upcoming instruction
   *                   this value can be re-set to none.
   *  -> Subscribers:  Determines the list of instructions which operands must be updated to available, once the data
   *                   in the register is written. When a register is currently in write state (used), readers must wait
   *                   for the data to be available (Sid's thesis could change this behavior). Once the reader has finished
   *                   producing the value, all the operands in the subscribers list are updated to full.
   *  -> broadcast:    When a readwrite operation occurs on a register, one must be careful to keep both dependencies. Since 
   *                   we do not have a value broadcasting mechanism (i.e. tomasulo's bus), we must copy the value "manually"
   *                   from one register to the other. This occurs so the write can occur while other instructions read from the same
   *                   location. For example: 
   *                    1. Read R
   *                    2. Read R
   *                    3. Read R
   *                    4. ReadWrite R
   *                   in this case, in order for instruction 4 to begin it must wait for other instructions 1-3, otherwise, the write 
   *                   operation will change the values read by 1-3. What we do is we assign a new register to 4 (renaming) and then broadcast
   *                   the value from R to R'. This means. Copy the value from R to R'. Then R' can be freely modified
   * 
   *  Allowing scheduling means to change the empty_full bit to full. Once all operands have their operands full, the instruction state
   *  is changed to READY
   * 
   * When the instruction is finished, the following table determines the actions to take
   * 
   * +-------------+---------+------------------------------+
   * | register is | used is | Result                       |
   * +-------------+---------+------------------------------+
   * | Read        | Read    | remove inst from subscribers |
   * |             |         | if (len(subscribers) == 0)   |
   * |             |         |   used = none                |
   * +-------------+---------+------------------------------+
   * | Read        | write   | ERROR!!                      |
   * +-------------+---------+------------------------------+
   * | Write       | write   | if (len(subscribers)>0)      |
   * |             |         |   used = read                |
   * |             |         |   for each subscriber:       |
   * |             |         |     mark op as ready         |
   * |             |         |   for each broadcast:        |
   * |             |         |     copy regiser             |
   * |             |         | else                         |
   * |             |         |   used = none                |
   * +-------------+---------+------------------------------+
   * | Write       | Read    | ERROR!!                      |
   * +-------------+---------+------------------------------+
   * | ReadWrite   | Write   | if (len(subscribers)>0)      |
   * |             |         |   used = read                |
   * |             |         |   for each subscriber:       |
   * |             |         |     mark op as ready         |
   * |             |         |   for each broadcast:        |
   * |             |         |     copy regiser             |
   * |             |         | else                         |
   * |             |         |   used = none                |
   * +-------------+---------+------------------------------+
   * | ReadWrite   | Read    | ERROR!!                      |
   * +-------------+---------+------------------------------+
   * 
   * Structural hazzards:
   * ====================
   * 
   * When attempting register renaming there is a chance that no register can be reserved. This is,
   * the register file is completely full for that given size. When this happens it is an structural
   * hazzard and it can only be resolved if finished instructions are processed. However, the SU (who calls
   * checMarkInstructionsToSched) is also the one that handles instruction completion. therefore we must return
   * control flow to the caller, before trying to resolve the hazzard.
   * 
   * To do this we keep the instruction state, set it as hazzardous, and we keep the current operand number;   
   * The operand number is kept to avoid re-evaluating previous operands that have already been added to the 
   * other data structures (have already been processed). On re-entry to checkMarkInstructionsToSched, we check
   * if there is a previous hazzard, and we ignore current instruction in favor of resolving the hazzard. 
   */

#define reg_state_str(state) (state == reg_state::READ ? std::string("reg_state::READ") : state == reg_state::WRITE ? std::string("reg_state::WRITE"): state == reg_state::READWRITE ? std::string("reg_state::READWRITE") :  std::string("reg_state::NONE"))
  class ilp_OoO {
    private:
      reg_file_module hidden_register_file;
      enum reg_state { NONE, READ, WRITE, READWRITE };

      // We must maintain a reference to the operand and its specific operand. The operand is maintained
      // as an integer to be used with getOp(). The instruction is needed to allow changing instruction state
      typedef std::pair<instruction_state_pair *, int> instruction_operand_pair_t;
      typedef std::vector< instruction_operand_pair_t > instruction_operand_ref_t;

      memory_queue_controller memCtrl;
      std::unordered_map<decoded_reg_t, reg_state> used;
      std::unordered_map<decoded_reg_t, decoded_reg_t> registerRenaming;
      std::unordered_set<decoded_reg_t> renamedInUse;
      std::map<decoded_reg_t, instruction_operand_ref_t> subscribers;
      std::map<decoded_reg_t, instruction_operand_ref_t> broadcasters;
      std::set<memory_location> memoryLocations;
      std::set<instruction_state_pair *> reservationTable; // Contains instructions that were already processed

      // Structural hazzard: When there are no registers for applying renaming. We must keep current progress 
      // on the instruction and attempt to continue in the process. Control flow must be returned to caller
      // to try to liberate registers
      instruction_state_pair * hazzard_inst_state;
      std::set<int> already_processed_operands;
      std::map<decoded_reg_t, reg_state> inst_operand_dir;

    public:
      ilp_OoO() : hazzard_inst_state(nullptr) { }
      /** \brief check if instruction can be scheduled 
      * Returns true if the instruction could be scheduled according to
      * the current detected hazards. If it is possible to schedule it, then
      * it will add the list of detected hazards to the tracking tables
      */
      bool inline checkMarkInstructionToSched(instruction_state_pair * inst_state) {

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
                    it_rename.first->second = newReg;
                  }
                  current_operand->value.reg = newReg;
                  // Insert it in used, and register to subscribers
                } else {
                  // Register not in used. Remove renaming for future references
                  auto it_rename = registerRenaming.find(current_operand->value.reg);
                  if (it_rename != registerRenaming.end()) {
                    registerRenaming.erase(it_rename);
                    renamedInUse.erase(it_rename->second);
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
                    it_rename.first->second = new_renamed_reg;
                  }
                } else {
                  if (it_rename != registerRenaming.end()) {
                    registerRenaming.erase(it_rename);
                    renamedInUse.erase(it_rename->second);
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



      decoded_reg_t inline getRenamedRegister(decoded_reg_t & otherReg) {
        static uint32_t curRegNum = 0;
        decoded_reg_t newReg = otherReg;
        curRegNum = (curRegNum+1) % hidden_register_file.getNumRegForSize(newReg.reg_size_bytes);
        newReg.reg_number = curRegNum;
        uint32_t attempts = 0;

        // Iterate over the hidden register is found that is not being used
        do {
          newReg.reg_ptr = hidden_register_file.getNextRegister(newReg.reg_size_bytes, newReg.reg_number);
          attempts++;
        } while ((this->used.find(newReg) != this->used.end() || this->renamedInUse.find(newReg) != this->renamedInUse.end()) && attempts != hidden_register_file.getNumRegForSize(newReg.reg_size_bytes));
        if (attempts == hidden_register_file.getNumRegForSize(newReg.reg_size_bytes)) {
          SCMULATE_INFOMSG(4, "When trying to rename, we could not find another register that was free out of %d", hidden_register_file.getNumRegForSize(newReg.reg_size_bytes));
          return otherReg;
        }
        newReg.reg_name = std::string("R_ren_") + newReg.reg_size + std::string("_") + std::to_string(newReg.reg_number);
        SCMULATE_INFOMSG(4, "Register %s mapped to %s with renaming", otherReg.reg_name.c_str(), newReg.reg_name.c_str());
        return newReg;
      }



      void inline instructionFinished(instruction_state_pair * inst_state) {
        decoded_instruction_t * inst = inst_state->first;
        
        SCMULATE_INFOMSG(3, "Instruction (%lu) %s has finished. Starting clean up process", (unsigned long) inst,  inst->getFullInstruction().c_str() );
        // In memory instructions we need to remove range from memory
        if (inst->isMemoryInstruction()) {
          std::vector <memory_location> ranges = inst->getMemoryRange();
          for (auto it = ranges.begin(); it != ranges.end(); ++it)
            memCtrl.removeRange( *it );
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

      bool inline isInstructionReady(decoded_instruction_t * inst) {
        for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
          operand_t &thisOperand = inst->getOp(i);
          if(!thisOperand.full_empty) {
            return false;
          }
        }
        return true;
      }

      bool inline stallMemoryInstruction(decoded_instruction_t * inst) {
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
          std::vector <memory_location> ranges = inst->getMemoryRange();
          for (auto it = ranges.begin(); it != ranges.end(); ++it) {
            if (memCtrl.itOverlaps( *it )) {
              return true;
            }
          }
          // The instruction is ready to schedule, let's mark the ranges as busy
          for (auto it = ranges.begin(); it != ranges.end(); ++it)
            memCtrl.addRange( *it );
        }
        return false;

      }

      void getOperandsDirs (decoded_instruction_t * inst) {
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
  };

  class ilp_sequential {
    private:
      bool sequential_sw;
    public:
      ilp_sequential() : sequential_sw(true) {}
      bool inline checkMarkInstructionToSched(instruction_state_pair * inst_pair) {
        if (sequential_sw) {
          sequential_sw = false;
          inst_pair->second = instruction_state::READY;
          return true;
        }
        inst_pair->second = instruction_state::STALL;
        return false;
      }
      void inline instructionFinished() {
        sequential_sw = true;
      }
  };

  class ilp_controller {
      const ILP_MODES SCMULATE_ILP_MODE;
      ilp_sequential seq_ctrl;
      ilp_superscalar supscl_ctrl;
      ilp_OoO ooo_ctrl;
    public:
      ilp_controller (const ILP_MODES ilp_mode) : SCMULATE_ILP_MODE(ilp_mode) {
        SCMULATE_INFOMSG_IF(3, SCMULATE_ILP_MODE == ILP_MODES::SEQUENTIAL, "Using %d ILP_MODES::SEQUENTIAL",SCMULATE_ILP_MODE );
        SCMULATE_INFOMSG_IF(3, SCMULATE_ILP_MODE == ILP_MODES::SUPERSCALAR, "Using %d ILP_MODES::SUPERSCALAR", SCMULATE_ILP_MODE);
        SCMULATE_INFOMSG_IF(3, SCMULATE_ILP_MODE == ILP_MODES::OOO, "Using %d ILP_MODES::OOO", SCMULATE_ILP_MODE);
       }
      bool inline checkMarkInstructionToSched(instruction_state_pair * inst_pair) {
        if (SCMULATE_ILP_MODE == ILP_MODES::SEQUENTIAL) {
          return seq_ctrl.checkMarkInstructionToSched(inst_pair);
        }
        else if (SCMULATE_ILP_MODE  == ILP_MODES::SUPERSCALAR) {
          return supscl_ctrl.checkMarkInstructionToSched(inst_pair);
        }
        else if (SCMULATE_ILP_MODE == ILP_MODES::OOO) {
          return ooo_ctrl.checkMarkInstructionToSched(inst_pair);
        }
        else {
          SCMULATE_ERROR(0, "What are you doing here?");
        }
        return false;
      }
      void inline instructionFinished(instruction_state_pair * inst) {
        if (SCMULATE_ILP_MODE == ILP_MODES::SEQUENTIAL) {
          seq_ctrl.instructionFinished();
        } else if (SCMULATE_ILP_MODE  == ILP_MODES::SUPERSCALAR) {
          supscl_ctrl.instructionFinished(inst);
        } else if (SCMULATE_ILP_MODE == ILP_MODES::OOO) {
          ooo_ctrl.instructionFinished(inst);
        }
        else {
          SCMULATE_ERROR(0, "What are you doing here?");
        }
      }
  };

} // namespace scm
#endif