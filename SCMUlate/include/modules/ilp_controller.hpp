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
    unsigned char * reg_name_ptr;
    std::uint_fast16_t reg_direction;
    register_reservation() : reg_name_ptr(nullptr), reg_direction(OP_IO::NO_RD_WR) {}
    register_reservation(const register_reservation &other) : reg_name_ptr(other.reg_name_ptr), reg_direction(other.reg_direction) {}
    register_reservation(unsigned char * name, std::uint_fast16_t mask) : reg_name_ptr(name), reg_direction(mask) {}
    inline bool operator<(const register_reservation &other) const
    {
      return this->reg_name_ptr < other.reg_name_ptr;
    }
  };

  class memory_queue_controller {
    private:
      // Ranges are inclusive on begining and exclusive on end
      std::set<memory_location> ranges_read;
      std::set<memory_location> ranges_write;
    public:
      memory_queue_controller() { };
      uint32_t inline numberOfRanges () { return ranges_read.size() + ranges_write.size(); }
      void inline addRange( memranges_pair * in_ranges) {
        SCMULATE_INFOMSG(4, "Adding ranges");
        this->ranges_read.insert(in_ranges->reads.begin(), in_ranges->reads.end());
        this->ranges_write.insert(in_ranges->writes.begin(), in_ranges->writes.end());
      }
      void inline removeRanges(memranges_pair * out_ranges) {
        SCMULATE_INFOMSG(4, "Removing ranges");
        auto it_out = out_ranges->reads.begin();
        auto it = std::lower_bound(this->ranges_read.begin(), this->ranges_read.end(), *it_out);
        uint64_t firstsize = this->ranges_read.size();
        while (it != this->ranges_read.end() && it_out != out_ranges->reads.end()) {
            if (*it == *it_out) {
              it = this->ranges_read.erase(it);
              it_out++;
            } else{
                it++;
            }
        }
        uint64_t finalsize = this->ranges_read.size(); 

        SCMULATE_ERROR_IF(0, finalsize != firstsize - out_ranges->reads.size(), "Error deleting read ranges");

        firstsize = this->ranges_write.size();
        it_out = out_ranges->writes.begin();
        it = std::lower_bound(this->ranges_write.begin(), this->ranges_write.end(), *it_out);
        while (it != this->ranges_write.end() && it_out != out_ranges->writes.end()) {
            if (*it == *it_out) {
              it = this->ranges_write.erase(it);
              it_out++;
            } else{
                it++;
            }
        }
        finalsize = this->ranges_write.size(); 
        SCMULATE_ERROR_IF(0, finalsize != firstsize - out_ranges->writes.size(), "Error deleting write ranges");
      }
      bool inline itOverlaps(memranges_pair const * in_ranges) const{
        // We compare in_writes with writes, in_writes with reads, 
        // and in_reads with writes (allow read after read)
        // The start of the curLocation cannot be in between a beginning and end of the range
        // The end of the curLocation cannot be between a beginning and end
        
        if (itOverlapsHelper(&ranges_write, &in_ranges->writes))
          return true;
        if (itOverlapsHelper(&ranges_read, &in_ranges->writes))
          return true;
        if (itOverlapsHelper(&ranges_write, &in_ranges->reads))
          return true;
        return false;
      }

      bool inline itOverlapsHelper(std::set<memory_location> const * ranges, std::set<memory_location> const * in_ranges) const {
        for (const auto& incoming_range : *in_ranges) {
          // Case ranges is empty, no possible overlap
          if (ranges->size() == 0) return false;

          // get the lower_bound ( next closest element or equal) 
          // and the upper_bound (next closest element or end)
          auto it_ranges = ranges->equal_range(incoming_range);
          auto next_element = it_ranges.second;
          auto prev_eq_element = it_ranges.first;

          // Case I only have one element, or the incoming range is lower than
          // the minimum element
          if (prev_eq_element != ranges->begin() && *prev_eq_element != incoming_range)
            prev_eq_element = std::prev(prev_eq_element);
          else if (incoming_range < *prev_eq_element)
            prev_eq_element = ranges->end();

          if (prev_eq_element != ranges->end() && (*prev_eq_element == incoming_range || prev_eq_element->upperLimit() > incoming_range.memoryAddress)) {
            return true;
          }
          if (next_element != ranges->end() && next_element->memoryAddress < incoming_range.upperLimit()) {
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

        if (inst->getOp1().type == operand_t::REGISTER && hazardExist(inst->getOp1().value.reg.reg_ptr, (inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR)))) {
          inst_state->second = instruction_state::STALL;
          return false;
        }
        if (inst->getOp2().type == operand_t::REGISTER && hazardExist(inst->getOp2().value.reg.reg_ptr, (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR))>>2)) {
          inst_state->second = instruction_state::STALL;
          return false;
        }
        if (inst->getOp3().type == operand_t::REGISTER && hazardExist(inst->getOp3().value.reg.reg_ptr, (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR))>>4)) {
          inst_state->second = instruction_state::STALL;
          return false;
        }

        // In memory instructions we need to figure out if there is a hazard in the memory
        if (inst->isMemoryInstruction()) {
          // Check all the ranges
          inst->calculateMemRanges();
          memranges_pair * ranges = inst->getMemoryRange();
          if (memCtrl.itOverlaps( ranges )) {
            inst_state->second = instruction_state::STALL;
            return false;
          }

          // The instruction is ready to schedule, let's mark the ranges as busy
          memCtrl.addRange( ranges );
        }
        // Mark the registers
        if (inst->getOp1().type == operand_t::REGISTER) {
          uint_fast16_t io = inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR);
          SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp1().value.reg.reg_name.c_str(), io);
          register_reservation reserv(inst->getOp1().value.reg.reg_ptr, io);
          busyRegisters.insert(reserv);
        }
        if (inst->getOp2().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR)) >> 2;
          SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp2().value.reg.reg_name.c_str(), io);
          register_reservation reserv(inst->getOp2().value.reg.reg_ptr, io);
          busyRegisters.insert(reserv);          
          }
        if (inst->getOp3().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR)) >> 4;
          SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp3().value.reg.reg_name.c_str(), io);
          register_reservation reserv(inst->getOp3().value.reg.reg_ptr, io);
          busyRegisters.insert(reserv);          
          }
        inst_state->second = instruction_state::READY;
        return true;
      }

      void inline instructionFinished(instruction_state_pair * inst_state) {
        decoded_instruction_t * inst = inst_state->first;
        // In memory instructions we need to figure out if there is a hazard in the memory
        if (inst->isMemoryInstruction()) {
          memranges_pair * ranges = inst->getMemoryRange();
          if (ranges->reads.size() != 0 || ranges->writes.size() != 0)
            memCtrl.removeRanges( ranges ); 
        } 

        if (inst->getOp1().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR));
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX", inst->getOp1().value.reg.reg_name.c_str(), io );
          eraseReg(inst->getOp1().value.reg.reg_ptr, io);
        }
        if (inst->getOp2().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR)) >> 2;
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX",inst->getOp2().value.reg.reg_name.c_str(), io );
          eraseReg(inst->getOp2().value.reg.reg_ptr, io);
        }
        if (inst->getOp3().type == operand_t::REGISTER) {
          uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR)) >> 4;
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX",inst->getOp3().value.reg.reg_name.c_str(), io);
          eraseReg(inst->getOp3().value.reg.reg_ptr, io);
        }
        SCMULATE_INFOMSG(5, "The number of busy regs is %lu", this->busyRegisters.size());
      }

      bool inline hazardExist(unsigned char * regName, uint_fast16_t io_dir) { 
        auto foundReg = busyRegisters.find(register_reservation(regName, io_dir));
        bool isFound = busyRegisters.find(register_reservation(regName, io_dir)) != busyRegisters.end();
        SCMULATE_INFOMSG_IF(6, isFound, "Hazard detected");
        if (!isFound) return false;
        if ((foundReg->reg_direction & OP_IO::OP1_WR) | (io_dir & OP_IO::OP1_WR)) return true; // WAW or RAW or WAR
        return false;
      }
      void inline eraseReg(unsigned char * regName, uint_fast16_t io_dir) { 
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
   * | ReadWrite   | None          | None        | Used = write               | Allow Scheduling       |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | ReadWrite   | Read          | None        | rename = other             | Allow Scheduling       |
   * |             |               |             | operand = other            |                        |
   * |             |               |             | copy origianl to other     |                        |
   * |             |               |             | set other to write         |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | ReadWrite   | Write         | None        | if (subscribers)           | Don't allow            |
   * |             |               |             |   rename = other           |                        |
   * |             |               |             |   operand = other          |                        |
   * |             |               |             |   set other to write       |                        |
   * |             |               |             |   broadcast += other       |                        |
   * |             |               |             | else                       |                        |
   * |             |               |             |   broadcast += original    |                        |
   * |             |               |             |   // On finish dont change |                        |
   * |             |               |             |   // operand to read       |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | ReadWrite   | Read          | None        | rename = other             | Allow Scheduling       |
   * |             |               |             | operand = other            |                        |
   * |             |               |             | copy origianl to other     |                        |
   * |             |               |             | set other to write         |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | ReadWrite   | Read          | Set to X    | if (used(x) == write)      | Don't allow            |
   * |             | Can't be      |             |   if (subscribers(X))      |                        |
   * |             | write         |             |     rename = other         |                        |
   * |             |               |             |     operand = other        |                        |
   * |             |               |             |     set other to write     |                        |
   * |             |               |             |     broadcast += other     |                        |
   * |             |               |             |   else                     |                        |
   * |             |               |             |     // keep renaming       |                        |
   * |             |               |             |     rename = X             |                        |
   * |             |               |             |     broadcast += original  |                        |
   * |             |               |             | elif (used(x) ==  read)    |                        |
   * |             |               |             |   rename = other           |                        |
   * |             |               |             |   operand = other          |                        |
   * |             |               |             |   set other to write       |                        |
   * |             |               |             |   broadcast += other       |                        |
   * +-------------+---------------+-------------+----------------------------+------------------------+
   * | ReadWrite   | ANY           | Set to X    | if (!used(x))              | Allow scheduling       |
   * |             |               |             |   // keep renaming         |                        |
   * |             |               |             |   // keep renaming         |                        |
   * |             |               |             |   set other to write       |                        |
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
   *                   from one register to the other. This occurs so the RW operand can be written while other instructions read from the same
   *                   location. For example: 
   *                    1. Read R
   *                    2. Read R
   *                    3. Read R
   *                    4. ReadWrite R
   *                   in this case, in order for instruction 4 to begin it must wait for other instructions 1-3, otherwise, the write 
   *                   operation will change the values read by 1-3. What we do is we assign a new register to 4 (renaming) and then broadcast
   *                   the value from R to R'. This means. Copy the value from R to R'. Then R' can be freely modified.
   *                   It is possible to avoid broadcasting (save copy time), when there are no subscribers to a given register. For example
   *                    1. Write R
   *                    2. Readwrite R
   *                   In this case the value of R does not need to be saved for any reads. Therefore, we can bypass the broadcasting and
   *                   leave the original reference to R in the other op. 
   *                   There is a third case that must be considered. When two consecutive readwrites occur (with or without intermediate values)
   *                    1. Write R
   *                    2. ReadWrite R
   *                    3. ReadWrite R
   *                    4. Read R
   *                    5. Read R
   *                    6. ReadWrite R
   *                   In this case one must be careful not to broadcast the original write incorrecty. The order in which the broadcast are
   *                   registered is important. While 1 is happening 2 is issued. When analyzing 2, a broadcast operation on R is registered, 
   *                   but no renaming occurs (bypassed broadcastig). In this case when 1 is finished R will not be set to read, insteaad, it is
   *                   kept in write while 2 is enabled. If 3 is analyzed before 1 finishes, 3 will realize that it can bypass the broadcast
   *                   however, the broadcast must occur from 2 (the dependency between 2 and 3 must be kept). Therefore, the broadcast list after
   *                   3 is analyzed (and 1 has not finished) is ([inst2, op1(R)],[inst3,op1(R)]). Broadcasting must stop after the first broadcast.
   *                   For the case of instruction 6, the subscribed instructions 4 and 5 will force renaming. Broadcasting will be on register R'.
   *                   The list of broadcasting after 6 is issued (and 1 has not finished) is ([inst2, op1(R)],[inst3, Op1(R)],[inst6, op1(R')]). While the 
   *                   subscribers of R will be ([inst4, op1(R)], [inst5, op1(R)])
   *                   ()
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
   * |             |         |   for each broadcast:        |
   * |             |         |     if (R == Rb):            |
   * |             |         |       //enable this and any  |
   * |             |         |       //future broadcasters  |
   * |             |         |       //with the same R value|
   * |             |         |     else:                    |
   * |             |         |       copy regiser           |
   * |             |         |   if (broadcasted_all):      |
   * |             |         |     used = read              |
   * |             |         |     for each subscriber:     |
   * |             |         |       mark op as ready       |
   * |             |         | else:                        |
   * |             |         |   used = none                |
   * +-------------+---------+------------------------------+
   * | Write       | Read    | ERROR!!                      |
   * +-------------+---------+------------------------------+
   * | ReadWrite   | Write   | if (len(subscribers)>0)      |
   * |             |         |   for each broadcast:        |
   * |             |         |     if (R == Rb):            |
   * |             |         |       //enable this and any  |
   * |             |         |       //future broadcasters  |
   * |             |         |       //with the same R value|
   * |             |         |     else:                    |
   * |             |         |       copy regiser           |
   * |             |         |   if (broadcasted_all):      |
   * |             |         |     used = read              |
   * |             |         |     for each subscriber:     |
   * |             |         |       mark op as ready       |
   * |             |         | else:                        |
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
  class ilp_OoO {
    private:
      reg_file_module * hidden_register_file;

      // We must maintain a reference to the operand and its specific operand. The operand is maintained
      // as an integer to be used with getOp(). The instruction is needed to allow changing instruction state
      typedef std::pair<instruction_state_pair *, int> instruction_operand_pair_t;
      typedef std::vector< instruction_operand_pair_t > instruction_operand_ref_t;

      memory_queue_controller memCtrl;
      std::unordered_map<unsigned char *, reg_state> used;
      std::unordered_map<decoded_reg_t, decoded_reg_t> registerRenaming;
      std::unordered_set<unsigned char *> renamedInUse;
      std::map<unsigned char*, instruction_operand_ref_t> subscribers;
      std::map<unsigned char*, instruction_operand_ref_t> broadcasters;
      std::unordered_set<instruction_state_pair *> reservationTable; // Contains instructions that were already processed

      // Structural hazzard: When there are no registers for applying renaming. We must keep current progress 
      // on the instruction and attempt to continue in the process. Control flow must be returned to caller
      // to try to liberate registers
      instruction_state_pair * hazzard_inst_state;
      std::unordered_set<int> already_processed_operands;

    public:
      ilp_OoO() : hidden_register_file(new reg_file_module()), hazzard_inst_state(nullptr) { }
      /** \brief check if instruction can be scheduled 
      * Returns true if the instruction could be scheduled according to
      * the current detected hazards. If it is possible to schedule it, then
      * it will add the list of detected hazards to the tracking tables
      */
      bool checkMarkInstructionToSched(instruction_state_pair * inst_state);

      void inline printStats() {
        SCMULATE_INFOMSG(6,"%lu\t%lu\t%lu\t%lu\t%lu\t%lu\n", used.size(), registerRenaming.size(), renamedInUse.size(), subscribers.size(), broadcasters.size(), reservationTable.size());
      }

      decoded_reg_t inline getRenamedRegister(decoded_reg_t & otherReg);

      void instructionFinished(instruction_state_pair * inst_state);

      bool inline isInstructionReady(decoded_instruction_t * inst);

      bool inline stallMemoryInstruction(decoded_instruction_t * inst);

      ~ilp_OoO() {
        delete hidden_register_file;
      }
  };

  class ilp_sequential {
    private:
      bool sequential_sw;
    public:
      ilp_sequential() : sequential_sw(true) {}
      bool inline checkMarkInstructionToSched(instruction_state_pair * inst_pair) {
        if (inst_pair->first->isMemoryInstruction()){
          inst_pair->first->calculateMemRanges();
        }
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
      void printStats(){
        if (SCMULATE_ILP_MODE == ILP_MODES::OOO) {
          ooo_ctrl.printStats();
        }
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