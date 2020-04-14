#ifndef __ILP_CONTROLLER__
#define __ILP_CONTROLLER__

/** \brief Instruction Level Parallelism Controller 
 *
 * This file contains all the necessary definitions and book keeping to 
 * know if an instruction can be scheduled or not. It contains the needed tables that
 * keep track of data depeendencies and others. 
 */

#include "SCMUlate_tools.hpp"
#include "instructions.hpp"
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

  class ilp_controller {
    private:
      struct memory_location {
        l2_memory_t memoryAddress;
        uint32_t size;
        public:
        memory_location() : memoryAddress(0), size(0) {}
        memory_location(l2_memory_t memAddr, uint32_t nsize) : memoryAddress(memAddr), size(nsize) {}
        memory_location(const memory_location& other) : memoryAddress(other.memoryAddress), size(other.size) {}
        l2_memory_t upperLimit() const { return memoryAddress + size; }
        inline bool operator<(const memory_location& other) const {
          return this->memoryAddress < other.memoryAddress;
        }
        inline bool operator==(const memory_location& other) const {
          return (this->memoryAddress == other.memoryAddress && this->size == other.size);
        }
      };
      class memory_queue_controller {
        private:
          // Ranges are exclusive on begining and end
          std::set<memory_location> ranges;
        public:
          memory_queue_controller() {
          };
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
              if ( itRange->memoryAddress < curLocation.memoryAddress && itRange->upperLimit() > curLocation.memoryAddress) {
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
      memory_queue_controller memCtrl;
      std::set<std::string> busyRegisters;
      //std::queue<decoded_instruction_t> reservationTable;
      std::set<memory_location> memoryLocations;
    public:
      ilp_controller() { }
      /** \brief check if instruction can be scheduled 
      * Returns true if the instruction could be scheduled according to
      * the current detected hazards. If it is possible to schedule it, then
      * it will add the list of detected hazards to the tracking tables
      */
      bool inline checkMarkInstructionToSched(decoded_instruction_t * inst, bool markAsync = true) {
        if (inst->getType() == instType::COMMIT && (busyRegisters.size() != 0 || memCtrl.numberOfRanges() != 0))
          return false;
        // In memory instructions we need to figure out if there is a hazard in the memory
        if (inst->getType() == instType::MEMORY_INST) {
          // Check if the destination/source register is marked as busy
          int32_t size_dest = inst->getOp1().value.reg.reg_size_bytes;
          unsigned long base_addr = 0;
          unsigned long offset = 0;
          if (isRegBusy(inst->getOp1().value.reg.reg_name)) {
            return false;
          } else if (inst->getInstruction() != "LDIMM") {
            // Check for the memory address
            if (inst->getOp2().type == operand_t::IMMEDIATE_VAL) {
              // Load address immediate value
              base_addr = inst->getOp2().value.immediate;
            } else if (inst->getOp2().type == operand_t::REGISTER) {
              if (isRegBusy(inst->getOp2().value.reg.reg_name))
                return false;
              // Load address register value
              decoded_reg_t reg = inst->getOp2().value.reg;
              unsigned char * reg_ptr = reg.reg_ptr;
              int32_t size_reg_bytes = reg.reg_size_bytes;
              int32_t i, j;
              for (i = size_reg_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
                unsigned long temp = reg_ptr[i];
                temp <<= j*8;
                base_addr += temp;
              }
            } else {
              SCMULATE_ERROR(0, "Incorrect operand type");
              return false; // This may cause the system to get stuck
            }

            // Check for offset only on the instructions with such operand
            if (inst->getInstruction() == "LDOFF" || inst->getInstruction() == "STOFF") {
              if (inst->getOp3().type == operand_t::IMMEDIATE_VAL) {
                // Load address immediate value
                offset = inst->getOp3().value.immediate;
              } else if (inst->getOp3().type == operand_t::REGISTER) {
                if (isRegBusy(inst->getOp3().value.reg.reg_name))
                  return false;
                // Load address register value
                decoded_reg_t reg = inst->getOp3().value.reg;
                unsigned char * reg_ptr = reg.reg_ptr;
                int32_t size_reg_bytes = reg.reg_size_bytes;
                int32_t i, j;
                for (i = size_reg_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
                  unsigned long temp = reg_ptr[i];
                  temp <<= j*8;
                  offset += temp;
                }
              } else {
                SCMULATE_ERROR(0, "Incorrect operand type");
                return false; // This may cause the system to get stuck
              }
            }
            memory_location newRange (reinterpret_cast<l2_memory_t> (base_addr + offset), size_dest);
            if (memCtrl.itOverlaps( newRange ))
              return false;

            if (markAsync){
              // The instruction is ready to schedule
              memCtrl.addRange(newRange);
            }
          }

        } else {
          if (inst->getOp1().type == operand_t::REGISTER && isRegBusy(inst->getOp1().value.reg.reg_name))
            return false;
          if (inst->getOp2().type == operand_t::REGISTER && isRegBusy(inst->getOp2().value.reg.reg_name)) 
            return false;
          if (inst->getOp3().type == operand_t::REGISTER && isRegBusy(inst->getOp3().value.reg.reg_name)) 
            return false;
        }

        if (markAsync) {
          // Mark the registers
          if (inst->getOp1().type == operand_t::REGISTER) {
            SCMULATE_INFOMSG(5, "Mariking register %s as busy",inst->getOp1().value.reg.reg_name.c_str() );
            busyRegisters.insert(inst->getOp1().value.reg.reg_name);
          }
          if (inst->getOp2().type == operand_t::REGISTER) {
            SCMULATE_INFOMSG(5, "Mariking register %s as busy",inst->getOp2().value.reg.reg_name.c_str() );
            busyRegisters.insert(inst->getOp2().value.reg.reg_name);
          }
          if (inst->getOp3().type == operand_t::REGISTER) {
            SCMULATE_INFOMSG(5, "Mariking register %s as busy",inst->getOp3().value.reg.reg_name.c_str() );
            busyRegisters.insert(inst->getOp3().value.reg.reg_name);
          }
        }
        return true;
      }

      void instructionFinished(decoded_instruction_t * inst) {
        // In memory instructions we need to figure out if there is a hazard in the memory
        if (inst->getType() == instType::MEMORY_INST) {
          int32_t size_dest = inst->getOp1().value.reg.reg_size_bytes;
          unsigned long base_addr = 0;
          unsigned long offset = 0;
          if (inst->getInstruction() != "LDIMM") {
            // Check for the memory address
            if (inst->getOp2().type == operand_t::IMMEDIATE_VAL) {
              // Load address immediate value
              base_addr = inst->getOp2().value.immediate;
            } else if (inst->getOp2().type == operand_t::REGISTER) {
              // Load address register value
              decoded_reg_t reg = inst->getOp2().value.reg;
              unsigned char * reg_ptr = reg.reg_ptr;
              int32_t size_reg_bytes = reg.reg_size_bytes;
              int32_t i, j;
              for (i = size_reg_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
                unsigned long temp = reg_ptr[i];
                temp <<= j*8;
                base_addr += temp;
              }
            }

            // Check for offset only on the instructions with such operand
            if (inst->getInstruction() == "LDOFF" || inst->getInstruction() == "STOFF") {
              if (inst->getOp3().type == operand_t::IMMEDIATE_VAL) {
                // Load address immediate value
                offset = inst->getOp3().value.immediate;
              } else if (inst->getOp3().type == operand_t::REGISTER) {
                // Load address register value
                decoded_reg_t reg = inst->getOp3().value.reg;
                unsigned char * reg_ptr = reg.reg_ptr;
                int32_t size_reg_bytes = reg.reg_size_bytes;
                int32_t i, j;
                for (i = size_reg_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
                  unsigned long temp = reg_ptr[i];
                  temp <<= j*8;
                  offset += temp;
                }
              }
            }
            memory_location newRange (reinterpret_cast<l2_memory_t> (base_addr + offset), size_dest);
            memCtrl.removeRange( newRange );
          }
        }

        if (inst->getOp1().type == operand_t::REGISTER) {
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy",inst->getOp1().value.reg.reg_name.c_str() );
          busyRegisters.erase(inst->getOp1().value.reg.reg_name);
        }
        if (inst->getOp2().type == operand_t::REGISTER) {
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy",inst->getOp2().value.reg.reg_name.c_str() );
          busyRegisters.erase(inst->getOp2().value.reg.reg_name);
        }
        if (inst->getOp3().type == operand_t::REGISTER) {
          SCMULATE_INFOMSG(5, "Unmariking register %s as busy",inst->getOp3().value.reg.reg_name.c_str() );
          busyRegisters.erase(inst->getOp3().value.reg.reg_name);
        }
        SCMULATE_INFOMSG(5, "The number of busy regs is %lu", this->busyRegisters.size());
      }

      bool inline isRegBusy(std::string& regName) { return busyRegisters.find(regName) != busyRegisters.end(); }
      /** \brief Remove hazards from table after an instruction has finished
      * Once an instruction has been detected to have finished, then we mark that 
      * instruction to be clean
      */
      // void propagateResult(decoded_instruction_t * inst) {

      // }

      // bool isReservationFull () {
      //   return reservationTable.size() >= RESERVATION_TABLE_SIZE;
      // }
      /** \brief Takes an instruction that is ready from the reservation table
       * 
      */
      // decoded_instruction_t * getReadyInstruction() {

      // }
  };

} // namespace scm
#endif