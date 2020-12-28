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
   * This mode supports overlapping of false dependencies: RAR, WAR, and WAW. It uses a hidden register file
   * 
   */
  // class ilp_OoO {
  //   private:
  //     memory_queue_controller memCtrl;
  //     std::set<register_reservation> busyRegisters;
  //     //std::queue<decoded_instruction_t> reservationTable;
  //     std::set<memory_location> memoryLocations;
  //   public:
  //     ilp_OoO() { }
  //     /** \brief check if instruction can be scheduled 
  //     * Returns true if the instruction could be scheduled according to
  //     * the current detected hazards. If it is possible to schedule it, then
  //     * it will add the list of detected hazards to the tracking tables
  //     */
  //     bool inline checkMarkInstructionToSched(decoded_instruction_t * inst, bool markAsync = true) {
  //       if (inst->getType() == instType::COMMIT && (busyRegisters.size() != 0 || memCtrl.numberOfRanges() != 0))
  //         return false;
  //       // In memory instructions we need to figure out if there is a hazard in the memory
  //       if (inst->getType() == instType::MEMORY_INST) {
  //         // Check if the destination/source register is marked as busy
  //         int32_t size_dest = inst->getOp1().value.reg.reg_size_bytes;
  //         unsigned long base_addr = 0;
  //         unsigned long offset = 0;
  //         if (hazardExist(inst->getOp1().value.reg.reg_name, inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR))) {
  //           return false;
  //         } else if (inst->getInstruction() != "LDIMM") {
  //           // Check for the memory address
  //           if (inst->getOp2().type == operand_t::IMMEDIATE_VAL) {
  //             // Load address immediate value
  //             base_addr = inst->getOp2().value.immediate;
  //           } else if (inst->getOp2().type == operand_t::REGISTER) {
  //             if (hazardExist(inst->getOp2().value.reg.reg_name, (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR))>>2))
  //               return false;
  //             // Load address register value
  //             decoded_reg_t reg = inst->getOp2().value.reg;
  //             unsigned char * reg_ptr = reg.reg_ptr;
  //             int32_t size_reg_bytes = reg.reg_size_bytes;
  //             int32_t i, j;
  //             for (i = size_reg_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
  //               unsigned long temp = reg_ptr[i];
  //               temp <<= j*8;
  //               base_addr += temp;
  //             }
  //           } else {
  //             SCMULATE_ERROR(0, "Incorrect operand type");
  //             return false; // This may cause the system to get stuck
  //           }

  //           // Check for offset only on the instructions with such operand
  //           if (inst->getInstruction() == "LDOFF" || inst->getInstruction() == "STOFF") {
  //             if (inst->getOp3().type == operand_t::IMMEDIATE_VAL) {
  //               // Load address immediate value
  //               offset = inst->getOp3().value.immediate;
  //             } else if (inst->getOp3().type == operand_t::REGISTER) {
  //               if (hazardExist(inst->getOp3().value.reg.reg_name, (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR))>>4))
  //                 return false;
  //               // Load address register value
  //               decoded_reg_t reg = inst->getOp3().value.reg;
  //               unsigned char * reg_ptr = reg.reg_ptr;
  //               int32_t size_reg_bytes = reg.reg_size_bytes;
  //               int32_t i, j;
  //               for (i = size_reg_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
  //                 unsigned long temp = reg_ptr[i];
  //                 temp <<= j*8;
  //                 offset += temp;
  //               }
  //             } else {
  //               SCMULATE_ERROR(0, "Incorrect operand type");
  //               return false; // This may cause the system to get stuck
  //             }
  //           }
  //           memory_location newRange (reinterpret_cast<l2_memory_t> (base_addr + offset), size_dest);
  //           if (memCtrl.itOverlaps( newRange ))
  //             return false;

  //           if (markAsync){
  //             // The instruction is ready to schedule
  //             memCtrl.addRange(newRange);
  //           }
  //         }

  //       } else {
  //         if (inst->getOp1().type == operand_t::REGISTER && hazardExist(inst->getOp1().value.reg.reg_name, (inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR))))
  //           return false;
  //         if (inst->getOp2().type == operand_t::REGISTER && hazardExist(inst->getOp2().value.reg.reg_name, (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR))>>2)) 
  //           return false;
  //         if (inst->getOp3().type == operand_t::REGISTER && hazardExist(inst->getOp3().value.reg.reg_name, (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR))>>4)) 
  //           return false;
  //       }

  //       if (markAsync) {
  //         // Mark the registers
  //         if (inst->getOp1().type == operand_t::REGISTER) {
  //           uint_fast16_t io = inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR);
  //           SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp1().value.reg.reg_name.c_str(), io);
  //           register_reservation reserv(inst->getOp1().value.reg.reg_name, io);
  //           busyRegisters.insert(reserv);
  //         }
  //         if (inst->getOp2().type == operand_t::REGISTER) {
  //           uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR)) >> 2;
  //           SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp2().value.reg.reg_name.c_str(), io);
  //           register_reservation reserv(inst->getOp2().value.reg.reg_name, io);
  //           busyRegisters.insert(reserv);          
  //           }
  //         if (inst->getOp3().type == operand_t::REGISTER) {
  //           uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR)) >> 4;
  //           SCMULATE_INFOMSG(5, "Mariking register %s as busy with IO %lX", inst->getOp3().value.reg.reg_name.c_str(), io);
  //           register_reservation reserv(inst->getOp3().value.reg.reg_name, io);
  //           busyRegisters.insert(reserv);          
  //           }
  //       }
  //       return true;
  //     }

  //     void inline instructionFinished(decoded_instruction_t * inst) {
  //       // In memory instructions we need to figure out if there is a hazard in the memory
  //       if (inst->getType() == instType::MEMORY_INST) {
  //         int32_t size_dest = inst->getOp1().value.reg.reg_size_bytes;
  //         unsigned long base_addr = 0;
  //         unsigned long offset = 0;
  //         if (inst->getInstruction() != "LDIMM") {
  //           // Check for the memory address
  //           if (inst->getOp2().type == operand_t::IMMEDIATE_VAL) {
  //             // Load address immediate value
  //             base_addr = inst->getOp2().value.immediate;
  //           } else if (inst->getOp2().type == operand_t::REGISTER) {
  //             // Load address register value
  //             decoded_reg_t reg = inst->getOp2().value.reg;
  //             unsigned char * reg_ptr = reg.reg_ptr;
  //             int32_t size_reg_bytes = reg.reg_size_bytes;
  //             int32_t i, j;
  //             for (i = size_reg_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
  //               unsigned long temp = reg_ptr[i];
  //               temp <<= j*8;
  //               base_addr += temp;
  //             }
  //           }

  //           // Check for offset only on the instructions with such operand
  //           if (inst->getInstruction() == "LDOFF" || inst->getInstruction() == "STOFF") {
  //             if (inst->getOp3().type == operand_t::IMMEDIATE_VAL) {
  //               // Load address immediate value
  //               offset = inst->getOp3().value.immediate;
  //             } else if (inst->getOp3().type == operand_t::REGISTER) {
  //               // Load address register value
  //               decoded_reg_t reg = inst->getOp3().value.reg;
  //               unsigned char * reg_ptr = reg.reg_ptr;
  //               int32_t size_reg_bytes = reg.reg_size_bytes;
  //               int32_t i, j;
  //               for (i = size_reg_bytes-1, j = 0; j < 8 || i >= 0; --i, ++j ) {
  //                 unsigned long temp = reg_ptr[i];
  //                 temp <<= j*8;
  //                 offset += temp;
  //               }
  //             }
  //           }
  //           memory_location newRange (reinterpret_cast<l2_memory_t> (base_addr + offset), size_dest);
  //           memCtrl.removeRange( newRange );
  //         }
  //       }

  //       if (inst->getOp1().type == operand_t::REGISTER) {
  //         uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP1_RD | OP_IO::OP1_WR));
  //         SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX", inst->getOp1().value.reg.reg_name.c_str(), io );
  //         eraseReg(inst->getOp1().value.reg.reg_name, io);
  //       }
  //       if (inst->getOp2().type == operand_t::REGISTER) {
  //         uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP2_RD | OP_IO::OP2_WR)) >> 2;
  //         SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX",inst->getOp2().value.reg.reg_name.c_str(), io );
  //         eraseReg(inst->getOp2().value.reg.reg_name, io);
  //       }
  //       if (inst->getOp3().type == operand_t::REGISTER) {
  //         uint_fast16_t io = (inst->getOpIO() & (OP_IO::OP3_RD | OP_IO::OP3_WR)) >> 4;
  //         SCMULATE_INFOMSG(5, "Unmariking register %s as busy with IO %lX",inst->getOp3().value.reg.reg_name.c_str(), io);
  //         eraseReg(inst->getOp3().value.reg.reg_name, io);
  //       }
  //       SCMULATE_INFOMSG(5, "The number of busy regs is %lu", this->busyRegisters.size());
  //     }

  //     bool inline hazardExist(std::string& regName, uint_fast16_t io_dir) { 
  //       auto foundReg = busyRegisters.find(register_reservation(regName, io_dir));
  //       bool isFound = busyRegisters.find(register_reservation(regName, io_dir)) != busyRegisters.end();
  //       SCMULATE_INFOMSG_IF(5, isFound, "Hazard detected");
  //       if (!isFound) return false;
  //       if ((foundReg->reg_direction & OP_IO::OP1_WR) | (io_dir & OP_IO::OP1_WR)) return true; // WAW or RAW or WAR
  //       return false;
  //     }
  //     void inline eraseReg(std::string& regName, uint_fast16_t io_dir) { 
  //      busyRegisters.erase(register_reservation(regName, io_dir));
  //     }
  // };

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
      ilp_sequential seq_ctrl;
      ilp_superscalar supscl_ctrl;
    public:
      ilp_controller (ILP_MODES ilp_mode) : SCMULATE_ILP_MODE(ilp_mode) {
        SCMULATE_INFOMSG_IF(3, SCMULATE_ILP_MODE == ILP_MODES::SEQUENTIAL, "Using %d ILP_MODES::SEQUENTIAL",SCMULATE_ILP_MODE );
        SCMULATE_INFOMSG_IF(3, SCMULATE_ILP_MODE == ILP_MODES::SUPERSCALAR, "Using %d ILP_MODES::SUPERSCALAR", SCMULATE_ILP_MODE);
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