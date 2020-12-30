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
      // Ranges are exclusive on begining and end
      std::set<memory_location> ranges;
    public:
      memory_queue_controller() { };
      uint32_t inline numberOfRanges () { return ranges.size(); }
      void inline addRange(memory_location& curLocation) {
        SCMULATE_INFOMSG(0, "Adding range [%lu. %lu]",(unsigned long)curLocation.memoryAddress, (unsigned long)curLocation.memoryAddress+curLocation.size );
        this->ranges.insert(curLocation);
      }
      void inline removeRange(memory_location& curLocation) {
        SCMULATE_INFOMSG(0, "Adding range [%lu. %lu]",(unsigned long)curLocation.memoryAddress, (unsigned long)curLocation.memoryAddress+curLocation.size );
        this->ranges.erase(curLocation);
      }
      bool inline itOverlaps(memory_location& curLocation) {
        // The start of the curLocation cannot be in between a beginning and end of the range
        // The end of the curLocation cannot be between a beginning and end
        auto itRange = ranges.begin();
        bool res = false;
        for (; itRange != ranges.end(); itRange++) {
          if (itRange->memoryAddress < curLocation.memoryAddress && itRange->upperLimit() > curLocation.memoryAddress) {
            res = true;
            break;
          }
          if (itRange->memoryAddress < curLocation.upperLimit() && itRange->upperLimit() > curLocation.upperLimit()) {
            res = true;
            break;
          }
          if (itRange->memoryAddress > curLocation.upperLimit() && itRange->upperLimit() > curLocation.upperLimit())
            break;
        }
        return res;
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
        if (inst->getType() == instType::MEMORY_INST) {
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
        if (inst->getType() == instType::MEMORY_INST) {
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
   * | Read        | Any or None   | Set to X    | operand = X                | repeat process         |
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
   * | ReadWrite   | Any or None   | Set to X    | Operand = x                | Repeat Process         |
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
   *                   producing the value, all the operands in the suscribers list are updated to full.
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

#define reg_state_str(state) (state == reg_state::READ ? "reg_state::READ" : state == reg_state::WRITE ? "reg_state::WRITE" : state == reg_state::READWRITE ? "reg_state::READWRITE" : "reg_state::NONE")
  class ilp_OoO {
    private:
      inst_mem_module * inst_mem;
      enum reg_state { NONE, READ, WRITE, READWRITE };

      // We must maintain a reference to the operand and its specific operand. The operand is maintained
      // as an integer to be used with getOp(). The instruction is needed to allow changing instruction state
      typedef std::pair<instruction_state_pair *, int> instruction_operand_pair_t;
      typedef std::vector< instruction_operand_pair_t > instruction_operand_ref_t;

      memory_queue_controller memCtrl;
      std::map<decoded_reg_t, reg_state> used;
      std::map<decoded_reg_t, decoded_reg_t> registerRenaming;
      std::map<decoded_reg_t, instruction_operand_ref_t> subscribers;
      std::map<decoded_reg_t, instruction_operand_ref_t> broadcasters;
      std::set<memory_location> memoryLocations;
      std::set<instruction_state_pair *> reservationTable; // Contains instructions that were already processed

      // Structural hazzard: When there are no registers for applying renaming. We must keep current progress 
      // on the instruction and attempt to continue in the process. Control flow must be returned to caller
      // to try to liberate registers
      instruction_state_pair * hazzard_inst_state;
      std::set<int> processed_operands_keep;


    public:
      ilp_OoO(inst_mem_module * inst_mem_m) : inst_mem(inst_mem_m), hazzard_inst_state(nullptr) { }
      /** \brief check if instruction can be scheduled 
      * Returns true if the instruction could be scheduled according to
      * the current detected hazards. If it is possible to schedule it, then
      * it will add the list of detected hazards to the tracking tables
      */
      bool inline checkMarkInstructionToSched(instruction_state_pair * inst_state) {

        std::set<int> already_processed_operands;
        // If there is a structural hazzar, we must solve it first
        if (hazzard_inst_state != nullptr) {
          inst_state = hazzard_inst_state;
          already_processed_operands = processed_operands_keep;
        } else if (reservationTable.find(inst_state) != reservationTable.end()) {
          // Check if we have already processed the instructions. If so, just return
          return false;
        } else {
          reservationTable.insert(inst_state); 
        }
        decoded_instruction_t * inst = (inst_state->first);

        if (inst->getType() == instType::COMMIT) {
          if (used.size() != 0 || memCtrl.numberOfRanges() != 0) {
            inst_state->second = instruction_state::STALL;
            return false;
          } else {
            inst_state->second = instruction_state::READY;
            return true;
          }
        }

        SCMULATE_INFOMSG(5, "Starting dependency discovery and hazzard avoidance process");
        // Get directions for the current instruction. (Col 1)
        std::map<decoded_reg_t, reg_state> inst_operand_dir = getOperandsDirs(inst);
        // Let's analyze each operand's dependency. Marking as allow sched or not. 
        for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
          if (already_processed_operands.find(i) == already_processed_operands.end()) {
            operand_t &thisOperand = inst->getOp(i);
            if (thisOperand.type == operand_t::REGISTER) {
              auto it_inst_dir = inst_operand_dir.find(thisOperand.value.reg);

              bool process_finished = false;
              while (!process_finished) {
                process_finished = true;
                if (it_inst_dir->second == reg_state::READ) {
                  auto it_rename = registerRenaming.find(thisOperand.value.reg);
                  auto it_used = used.find(thisOperand.value.reg);

                  if (it_rename != registerRenaming.end()) {
                    SCMULATE_INFOMSG(5, "Register %s was renamed to %s", thisOperand.value.reg.reg_name.c_str(), it_rename->second.reg_name.c_str());
                    // Renamig = X, then we rename operand and start process again
                    thisOperand.value.reg = it_rename->second;
                    process_finished = false;
                    continue;
                  } else if(it_used != used.end() && it_used->second == reg_state::WRITE) {
                    // Attempt inserting if it does not exists already
                    auto it_subs_insert = subscribers.insert(
                                          std::pair<decoded_reg_t, instruction_operand_ref_t >(
                                                thisOperand.value.reg, instruction_operand_ref_t() ));
                    it_subs_insert.first->second.push_back(instruction_operand_pair_t(inst_state, i));
                    SCMULATE_INFOMSG(5, "Register %s is currently on WRITE state. Will subscribe but not sched", thisOperand.value.reg.reg_name.c_str());
                  } else if (it_used != used.end() && it_used->second == reg_state::READ) {
                    // Attempt inserting if it does not exists already
                    auto it_subs_insert = subscribers.insert(
                                          std::pair<decoded_reg_t, instruction_operand_ref_t >(
                                                thisOperand.value.reg, instruction_operand_ref_t() ));
                    it_subs_insert.first->second.push_back(instruction_operand_pair_t(inst_state, i));
                    // Operand ready for execution
                    thisOperand.full_empty = true;
                    SCMULATE_INFOMSG(5, "Register %s is currently on READ state. Subscribing and allowing sched", thisOperand.value.reg.reg_name.c_str());
                  } else {
                    // Insert it in used, and register to subscribers
                    used.insert(std::pair<decoded_reg_t, reg_state> (thisOperand.value.reg, reg_state::READ));
                    auto it_subs_insert = subscribers.insert(
                                          std::pair<decoded_reg_t, instruction_operand_ref_t >(
                                                thisOperand.value.reg, instruction_operand_ref_t() ));
                    it_subs_insert.first->second.push_back(instruction_operand_pair_t(inst_state, i));
                    SCMULATE_INFOMSG(5, "Register %s was not found in the 'used' registers map. Marking as READ, subscribing, and allowing sched", thisOperand.value.reg.reg_name.c_str());
                    // Operand ready for execution
                    thisOperand.full_empty = true;
                  }

                } else if (it_inst_dir->second == reg_state::WRITE) {
                  auto it_used = used.find(thisOperand.value.reg);
                  decoded_reg_t original_reg = thisOperand.value.reg;

                  if(it_used != used.end()) {
                    // Register is in use. Let's rename it
                    decoded_reg_t newReg = getRenamedRegister(thisOperand.value.reg);
                    // Check if renaming was successful.
                    if (newReg == thisOperand.value.reg) {
                      SCMULATE_ERROR(0, "STRUCTURAL HAZZARD on operand %d. No new register was found for renaming. Leaving other operands for later SU iteration.", i);
                      hazzard_inst_state = inst_state;
                      processed_operands_keep = already_processed_operands;
                      inst_state->second = instruction_state::STALL;
                      return false;
                    }

                    auto it_rename = registerRenaming.insert(std::pair<decoded_reg_t, decoded_reg_t> (thisOperand.value.reg, newReg));
                    SCMULATE_INFOMSG(5, "Register %s is being 'used'. Renaming it to %s", thisOperand.value.reg.reg_name.c_str(), newReg.reg_name.c_str());
                    if (!it_rename.second) {
                      SCMULATE_INFOMSG(5, "Register %s was renamed to %s now it is %s", thisOperand.value.reg.reg_name.c_str(), it_rename.first->second.reg_name.c_str(), newReg.reg_name.c_str());
                      it_rename.first->second = newReg;
                    }
                                        
                    thisOperand.value.reg = newReg;
                    // Insert it in used, and register to subscribers
                  } else {
                    // Register not in used. Remove renaming for future references
                    auto it_rename = registerRenaming.find(thisOperand.value.reg);
                    if (it_rename != registerRenaming.end())
                      registerRenaming.erase(it_rename);
                    SCMULATE_INFOMSG(5, "Register %s register was not found in the 'used' registers map. If renamed is set, we cleared it", thisOperand.value.reg.reg_name.c_str());
                  }

                  // Search for all the other operands of the same, and apply changes to avoid conflicts. Only add once
                  for (int other_op_num = 1; other_op_num <= MAX_NUM_OPERANDS; ++other_op_num) {
                    if (other_op_num != i && already_processed_operands.find(other_op_num) == already_processed_operands.end()) {
                      operand_t other_op = inst->getOp(other_op_num);
                      if (other_op.value.reg == original_reg) {
                        other_op.value.reg = thisOperand.value.reg;
                        other_op.full_empty = true;
                      }
                      already_processed_operands.insert(other_op_num);
                    }
                  }
                  SCMULATE_INFOMSG(5, "Register %s. Marking as WRITE and allowing sched", thisOperand.value.reg.reg_name.c_str());
                  used.insert(std::pair<decoded_reg_t, reg_state> (thisOperand.value.reg, reg_state::WRITE));
                  thisOperand.full_empty = true;

                } else if (it_inst_dir->second == reg_state::READWRITE) {
                  auto it_rename = registerRenaming.find(thisOperand.value.reg);
                  auto it_used = used.find(thisOperand.value.reg);
                  decoded_reg_t original_reg = thisOperand.value.reg;

                  if (it_rename != registerRenaming.end()) {
                    SCMULATE_INFOMSG(5, "Register %s was renamed to %s", thisOperand.value.reg.reg_name.c_str(), it_rename->second.reg_name.c_str());
                    // Rename all of the same to apply the same operations to all the same operands
                    for (int other_op_num = 1; other_op_num <= MAX_NUM_OPERANDS; ++other_op_num) {
                      if (already_processed_operands.find(other_op_num) == already_processed_operands.end()) {
                        operand_t other_op = inst->getOp(other_op_num);
                        if (other_op.value.reg == original_reg) {
                          other_op.value.reg = it_rename->second;
                        }
                      }
                    }
                    process_finished = false;
                    continue;
                  } else if(it_used != used.end() && it_used->second == reg_state::WRITE) {
                    // It is already being used, let's rename but not allow scheduling
                    decoded_reg_t newReg = getRenamedRegister(thisOperand.value.reg);
                    // Check if renaming was successful.
                    if (newReg == thisOperand.value.reg) {
                      SCMULATE_ERROR(0, "STRUCTURAL HAZZARD on operand %d. No new register was found for renaming. Leaving other operands for later SU iteration.", i);
                      hazzard_inst_state = inst_state;
                      processed_operands_keep = already_processed_operands;
                      inst_state->second = instruction_state::STALL;
                      return false;
                    }
                    auto it_rename = registerRenaming.insert(std::pair<decoded_reg_t, decoded_reg_t> (thisOperand.value.reg, newReg));
                    SCMULATE_INFOMSG(5, "Register %s is being 'used'. Renaming it to %s", thisOperand.value.reg.reg_name.c_str(), newReg.reg_name.c_str());
                    if (!it_rename.second) {
                      SCMULATE_INFOMSG(5, "Register %s was renamed to %s now it is %s", thisOperand.value.reg.reg_name.c_str(), it_rename.first->second.reg_name.c_str(), newReg.reg_name.c_str());
                      it_rename.first->second = newReg;
                    }
                    // Rename all of the same to apply the same operations to all the same operands
                    for (int other_op_num = 1; other_op_num <= MAX_NUM_OPERANDS; ++other_op_num) {
                      if (already_processed_operands.find(other_op_num) == already_processed_operands.end()) {
                        operand_t other_op = inst->getOp(other_op_num);
                        if (other_op.value.reg == original_reg) {
                          other_op.value.reg = newReg;
                        }
                        // Insert it in used, and register to broadcasters 
                        auto it_broadcast_insert = broadcasters.insert(
                                              std::pair<decoded_reg_t, instruction_operand_ref_t >(
                                                    thisOperand.value.reg, instruction_operand_ref_t() ));
                        it_broadcast_insert.first->second.push_back(instruction_operand_pair_t(inst_state, other_op_num));
                        already_processed_operands.insert(other_op_num);
                      }
                    }
                    SCMULATE_INFOMSG(5, "Register %s. Marking as WRITE. Do not allow to schedule until broadcast happens", thisOperand.value.reg.reg_name.c_str());
                    used.insert(std::pair<decoded_reg_t, reg_state> (thisOperand.value.reg, reg_state::WRITE));
                    // Attempt inserting if it does not exists already
                  } else if (it_used != used.end() && it_used->second == reg_state::READ) {
                    // It is already being used, let's rename
                    decoded_reg_t newReg = getRenamedRegister(thisOperand.value.reg);
                    // Check if renaming was successful.
                    if (newReg == thisOperand.value.reg) {
                      SCMULATE_ERROR(0, "STRUCTURAL HAZZARD on operand %d. No new register was found for renaming. Leaving other operands for later SU iteration.", i);
                      hazzard_inst_state = inst_state;
                      processed_operands_keep = already_processed_operands;
                      inst_state->second = instruction_state::STALL;
                      return false;
                    }
                    auto it_rename = registerRenaming.insert(std::pair<decoded_reg_t, decoded_reg_t> (thisOperand.value.reg, newReg));
                    SCMULATE_INFOMSG(5, "Register %s is being 'used'. Renaming it to %s", thisOperand.value.reg.reg_name.c_str(), newReg.reg_name.c_str());
                    if (!it_rename.second) {
                      SCMULATE_INFOMSG(5, "Register %s was renamed to %s now it is %s", thisOperand.value.reg.reg_name.c_str(), it_rename.first->second.reg_name.c_str(), newReg.reg_name.c_str());
                      it_rename.first->second = newReg;
                    }
                    // Rename all of the same to apply the same operations to all the same operands
                    for (int other_op_num = 1; other_op_num <= MAX_NUM_OPERANDS; ++other_op_num) {
                      if (already_processed_operands.find(other_op_num) == already_processed_operands.end()) {
                        operand_t other_op = inst->getOp(other_op_num);
                        if (other_op.value.reg == original_reg) {
                          other_op.value.reg = newReg;
                          other_op.full_empty = true;
                        }
                        already_processed_operands.insert(other_op_num);
                      }
                    }
                    std::memcpy(newReg.reg_ptr, thisOperand.value.reg.reg_ptr, thisOperand.value.reg.reg_size_bytes);
                    // Insert it in used, and register to subscribers
                    SCMULATE_INFOMSG(5, "Register %s. Marking as WRITE and allowing sched", thisOperand.value.reg.reg_name.c_str());
                    used.insert(std::pair<decoded_reg_t, reg_state> (thisOperand.value.reg, reg_state::WRITE));
                  } else {
                    // Enable all operands with the same register
                    for (int other_op_num = 1; other_op_num <= MAX_NUM_OPERANDS; ++other_op_num) {
                      if (already_processed_operands.find(other_op_num) == already_processed_operands.end()) {
                        operand_t other_op = inst->getOp(other_op_num);
                        if (other_op.value.reg == original_reg) {
                          other_op.full_empty = true;
                        }
                        already_processed_operands.insert(other_op_num);
                      }
                    }
                    // Insert it in used, and register to subscribers
                    used.insert(std::pair<decoded_reg_t, reg_state> (thisOperand.value.reg, reg_state::WRITE));
                    SCMULATE_INFOMSG(5, "Register %s was not found in the 'used' registers map. Marking as WRITE and allowing sched", thisOperand.value.reg.reg_name.c_str());
                  }

                }
              }
            } else {
              // Operand is ready by default (IMMEDIATE or UNKNOWN)
              thisOperand.full_empty = true;
            }
            already_processed_operands.insert(i);
          }
        }

        // We have finished processing this instruction, if it was a hazzard, it is not a hazzard anymore
        hazzard_inst_state = nullptr;

        // once each operand dependency is analyzed, we make a decision if waiting, ready or stall. 
        // The type of instruction plays an important role here. 
        bool inst_ready = isInstructionReady(inst);

        if (!inst_ready && inst->getType() == instType::CONTROL_INST || instType::MEMORY_INST) {
          inst_state->second = instruction_state::STALL;
          return false;
        }
        // In memory instructions we need to figure out if there is a hazard in the memory
        if (inst_ready && inst->getType() == instType::MEMORY_INST) {
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
        // Instruction ready for execution 
        inst_state->second = instruction_state::READY;
        return true;
      }

      decoded_reg_t inline getRenamedRegister(decoded_reg_t & otherReg) {
        reg_file_module * reg_file = this->inst_mem->getRegisterFileModule();
        decoded_reg_t newReg = otherReg;

        bool newRegFound = false;
        // Iterate over a register is found that is not being used
        while (!newRegFound) {
          newReg.reg_ptr = reg_file->getNextRegister(newReg.reg_size, newReg.reg_number);
          if (this->used.find(newReg) == this->used.end())
            newRegFound = true;
          // Worst case scenario, there are no more registers, we looked through all the registers
          if (newReg == otherReg) {
            SCMULATE_INFOMSG(4, "When trying to rename, we could not find another register that was free");
            break;
          }
        }
        return otherReg;
      }

      void inline instructionFinished(instruction_state_pair * inst_state) {

        /// REMINDERS:
        /// remove from reservation table

        decoded_instruction_t * inst = inst_state->first;

        // In memory instructions we need to figure out if there is a hazard in the memory
        if (inst->getType() == instType::MEMORY_INST) {
          std::vector <memory_location> ranges = inst->getMemoryRange();
          for (auto it = ranges.begin(); it != ranges.end(); ++it)
            memCtrl.removeRange( *it );
        }

        std::map<decoded_reg_t, reg_state> inst_operand_dir = getOperandsDirs(inst);
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
                  for (auto it_subs = subscribers_list_it->second.begin(); it_subs != subscribers_list_it->second.end(); it_subs++) {
                    if (it_subs->first->first == inst)
                      it_subs = subscribers_list_it->second.erase(it_subs);
                      SCMULATE_INFOMSG(5, "Removing subscription in register %s, for instruction %s, operand %d", it->first.reg_name.c_str(), inst->getFullInstruction().c_str(), it_subs->second);
                  }
                  // Move register to state NONE, if done
                  if (subscribers_list_it->second.size() == 0) {
                    subscribers.erase(subscribers_list_it);
                    used.erase(it_used);
                    SCMULATE_INFOMSG(0, "Subscriptions is empty. Move register %s to 'used' = NONE.", it->first.reg_name.c_str());
                  }
                } else {
                  SCMULATE_ERROR(0, "A read register should be subscribed to something");
                }
              } else {
                  SCMULATE_ERROR(0, "A read direction operand, should be 'used' marked as READ and it is %s", reg_state_str(it_used->second));
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
                    std::memcpy(other_inst_state_pair->first->getOp(it_broadcast->second).value.reg.reg_ptr, it->first.reg_ptr, it->first.reg_size_bytes);
                    // Marking as ready
                    other_inst_state_pair->first->getOp(it_broadcast->second).full_empty = true;
                    SCMULATE_INFOMSG(5, "Enabling operand %d with register %s, for instruction %s", i, it->first.reg_name.c_str(), other_inst_state_pair->first->getFullInstruction().c_str());
                    if (isInstructionReady(other_inst_state_pair->first)) {
                      other_inst_state_pair->second = instruction_state::READY;
                      SCMULATE_INFOMSG(5, "Marking instruction %s as READY", other_inst_state_pair->first->getFullInstruction().c_str());
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
                      other_inst_state_pair->second = instruction_state::READY;
                      SCMULATE_INFOMSG(5, "Marking instruction %s as READY", other_inst_state_pair->first->getFullInstruction().c_str());
                    }
                  }
                  // Marking the register as read, ready to be consumed
                  it_used->second = reg_state::READ;
                } else {
                  used.erase(it_used);
                  SCMULATE_INFOMSG(0, "Register had no subscriptions. Move register %s to 'used' = NONE.", it->first.reg_name.c_str());
                }
              } else {
                  SCMULATE_ERROR(0, "A write direction operand, should be 'used' marked as WRITE and it is %s", reg_state_str(it_used->second));
              }
            } else {
              SCMULATE_ERROR(0, "What are you doing here???");
            }

          }

          // We are done with this instruction, we remove it from the reservation tables
          auto it_restable = reservationTable.find(inst_state);
          reservationTable.erase(it_restable);
      }

      bool inline isInstructionReady(decoded_instruction_t * inst) {
        bool is_ready = true;
        for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
          operand_t &thisOperand = inst->getOp(i);
          if(!thisOperand.full_empty) {
            is_ready = false;
            break;
          }
        }
        return is_ready;
      }

      std::map<decoded_reg_t, reg_state> getOperandsDirs (decoded_instruction_t * inst) {
        std::map<decoded_reg_t, reg_state> return_map;
        for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
          if (inst->getOp(i).type == operand_t::REGISTER) {
            // Attempt to insert the register, if it is already there, the insert will return a pair with the iterator to the 
            // already inserted value.
            auto it_insert = return_map.insert(std::pair<decoded_reg_t, reg_state>(inst->getOp(i).value.reg, reg_state::NONE));
            reg_state &cur_state = it_insert.first->second;

            // We need to set the value of reg_state to read, write or readwrite
            if (inst->getOp(i).read && inst->getOp(i).write ||
                inst->getOp(i).read && cur_state == reg_state::WRITE ||
                inst->getOp(i).write && cur_state == reg_state::READ)
              cur_state = reg_state::READWRITE;
            else if (inst->getOp(i).write && cur_state == reg_state::NONE)
              cur_state = reg_state::WRITE;
            else if (inst->getOp(i).read && cur_state == reg_state::NONE)
              cur_state = reg_state::READ;
            
            SCMULATE_INFOMSG(5, "In instruction %s Register %s set as %s", inst->getFullInstruction().c_str() , inst->getOp(i).value.reg.reg_name.c_str(), reg_state_str(cur_state));
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
      ILP_MODES SCMULATE_ILP_MODE;
      inst_mem_module * inst_memory;
      ilp_sequential seq_ctrl;
      ilp_superscalar supscl_ctrl;
      ilp_OoO ooo_ctrl;
    public:
      ilp_controller (ILP_MODES ilp_mode, inst_mem_module * inst_mem_m) : SCMULATE_ILP_MODE(ilp_mode), inst_memory(inst_mem_m), ooo_ctrl(inst_memory) {
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
        else {
          SCMULATE_ERROR(0, "What are you doing here?");
        }
        return false;
      }
      void inline instructionFinished(instruction_state_pair * inst) {
        if (SCMULATE_ILP_MODE == ILP_MODES::SEQUENTIAL) {
          seq_ctrl.instructionFinished();
        }
        else if (SCMULATE_ILP_MODE  == ILP_MODES::SUPERSCALAR) {
          supscl_ctrl.instructionFinished(inst);
        }
        else {
          SCMULATE_ERROR(0, "What are you doing here?");
        }
      }
  };

} // namespace scm
#endif