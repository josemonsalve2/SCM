#ifndef __INSTRUCTIONS_DEF__
#define __INSTRUCTIONS_DEF__


/**
 * This file contains all the definitions of the regular expressions that allow to identify 
 * an instruction and its operands. It also defines the enum with the different instruction types
 * that is used by the SCM machine during fetch_decode of the program
 *
 * This file also defines the scm::instructions class that contains static methods that can be used
 * to identify the type of an instruction, as well as to extract the operands of the instruction
 *
 * The purpose of this file is to separate the definiton of the supported instructions
 * from the fetch_decode operation, as well as the instruction meomry operation
 *
 * TODO: This file should be replace by some sort of parser, but for now I have not definied the 
 * language correctly, and I am no front end expert, so this will have to do until then. 
 *
 */

#include <string>

#define REGISTER_REGEX "R[BbLl0-9_]+"
#define REGISTER_SPLIT_REGEX "R([BbLl0-9]+)_([0-9]+)"
#define INMIDIATE_REGEX "[-]?[0-9]+"
#define LABEL_REGEX "([a-zA-Z][a-zA-Z0-9_]*)"
#define COMMENT_REGEX "([ ]*//.*|^[ ]+$)"

#define DEF_INST(opcode, name, regExp, numOp, opInOut) {opcode, #name, regExp, numOp, opInOut}
#define NUM_OF_INST(a) sizeof(a)/sizeof(inst_def_t)

namespace scm {

  /** \brief Operands Intput Output descriptor bits
   *
   *  Contain the masks needed to define operands inputs and outputs for the out of order
   *  execution engine to work correctly, respecting data dependencies. 
   *
   */
  class OP_IO {
    public:
      static constexpr std::uint_fast16_t NO_RD_WR  { 0b0000'0000'0000'0000 }; // represents bit 0
      static constexpr std::uint_fast16_t OP1_RD  { 0b0000'0000'0000'0001 }; // represents bit 1
      static constexpr std::uint_fast16_t OP1_WR { 0b0000'0000'0000'0010 }; // represents bit 2
      static constexpr std::uint_fast16_t OP2_RD  { 0b0000'0000'0000'0100 }; // represents bit 3
      static constexpr std::uint_fast16_t OP2_WR { 0b0000'0000'0000'1000 }; // represents bit 4
      static constexpr std::uint_fast16_t OP3_RD  { 0b0000'0000'0001'0000 }; // represents bit 5
      static constexpr std::uint_fast16_t OP3_WR { 0b0000'0000'0010'0000 }; // represents bit 6
      static constexpr std::uint_fast16_t OP4_RD  { 0b0000'0000'0100'0000 }; // represents bit 7
      static constexpr std::uint_fast16_t OP4_WR { 0b0000'0000'1000'0000 }; // represents bit 8
      static constexpr std::uint_fast16_t OP5_RD  { 0b0000'0001'0000'0000 }; // represents bit 9
      static constexpr std::uint_fast16_t OP5_WR { 0b0000'0010'0000'0000 }; // represents bit 10
      static constexpr std::uint_fast16_t OP6_RD  { 0b0000'0100'0000'0000 }; // represents bit 11
      static constexpr std::uint_fast16_t OP6_WR { 0b0000'1000'0000'0000 }; // represents bit 12
      static constexpr std::uint_fast16_t OP7_RD  { 0b0001'0000'0000'0000 }; // represents bit 13
      static constexpr std::uint_fast16_t OP7_WR { 0b0010'0000'0000'0000 }; // represents bit 14
      static constexpr std::uint_fast16_t OP8_RD  { 0b0100'0000'0000'0000 }; // represents bit 15
      static constexpr std::uint_fast16_t OP8_WR { 0b1000'0000'0000'0000 }; // represents bit 16
  
      static constexpr std::uint_fast16_t getOpRDIO(int num) {
        if (num == 0) return NO_RD_WR;
        if (num == 1) return OP1_RD;
        if (num == 2) return OP2_RD;
        if (num == 3) return OP3_RD;
        if (num == 4) return OP4_RD;
        if (num == 5) return OP5_RD;
        if (num == 6) return OP6_RD;
        if (num == 7) return OP7_RD;
        if (num == 8) return OP8_RD;
        return NO_RD_WR;
      }
      static constexpr std::uint_fast16_t getOpWRIO(int num) {
        if (num == 0) return NO_RD_WR;
        if (num == 1) return OP1_WR;
        if (num == 2) return OP2_WR;
        if (num == 3) return OP3_WR;
        if (num == 4) return OP4_WR;
        if (num == 5) return OP5_WR;
        if (num == 6) return OP6_WR;
        if (num == 7) return OP7_WR;
        if (num == 8) return OP8_WR;
        return NO_RD_WR;
      }
  };

  class OP_ADDRESS {
    public:
      static constexpr std::uint_fast16_t NO_ADDRESS  { 0b0000'0000'0000'0000 }; // represents bit 0
      static constexpr std::uint_fast16_t OP1_IS_ADDRESS  { 0b0000'0000'0000'0001 }; // represents bit 1
      static constexpr std::uint_fast16_t OP2_IS_ADDRESS { 0b0000'0000'0000'0010 }; // represents bit 2
      static constexpr std::uint_fast16_t OP3_IS_ADDRESS  { 0b0000'0000'0000'0100 }; // represents bit 3
      static constexpr std::uint_fast16_t OP4_IS_ADDRESS { 0b0000'0000'0000'1000 }; // represents bit 4
      static constexpr std::uint_fast16_t OP5_IS_ADDRESS  { 0b0000'0000'0001'0000 }; // represents bit 5
      static constexpr std::uint_fast16_t OP6_IS_ADDRESS { 0b0000'0000'0010'0000 }; // represents bit 6
      static constexpr std::uint_fast16_t OP7_IS_ADDRESS  { 0b0000'0000'0100'0000 }; // represents bit 7
      static constexpr std::uint_fast16_t OP8_IS_ADDRESS { 0b0000'0000'1000'0000 }; // represents bit 8

  };

  typedef uint32_t opcode_t;

  /** \brief Instruction definition
   *
   *  This struct contains all the information needed for the definition of a instruction 
   *  from the language perspective
   *
   *  To add an instruction you must create it with DEF_INST() and then add it to the vector for 
   *  the corresponding group
   * 
   *  OPCODE:
   *  0b 0000 0000 
   *     GRP   ID
   *  Where group is:
   *  0x0_ -> SCM Specific Instructions
   *  0x1_ -> SCM Codelet Specific Instructions
   *  0x2_ -> Control instructions
   *  0x3_ -> Arithmetic instructions
   *  0x4_ -> Memory instructions
   *  0xFF -> INVALID INSTRUCTION
   */
  struct inst_def_t {
    const opcode_t opcode; // 1 bytes. msb => type, lsb =>ID
    std::string inst_name;
    std::string inst_regex;
    int num_op;
    std::uint_fast16_t op_in_out;
  };

  struct memory_location
  {
    l2_memory_t memoryAddress;
    uint32_t size;

    memory_location() : memoryAddress(0), size(0) {}
    memory_location(l2_memory_t memAddr, uint32_t nsize) : memoryAddress(memAddr), size(nsize) {}
    memory_location(const memory_location &other) : memoryAddress(other.memoryAddress), size(other.size) {}
    l2_memory_t upperLimit() const { return memoryAddress + size; }
    inline bool operator<(const memory_location &other) const
    {
      return this->memoryAddress < other.memoryAddress;
    }
    inline bool operator==(const memory_location &other) const
    {
      return (this->memoryAddress == other.memoryAddress && this->size == other.size);
    }
    inline bool operator!=(const memory_location &other) const
    {
      return !(*this == other);
    }
  };
  
  typedef struct {std::set<memory_location> reads; std::set<memory_location> writes;} memranges_pair;

  // SCM specific insctructions
  const inst_def_t COMMIT_INST = DEF_INST(0x00, COMMIT, "[ ]*(COMMIT;).*", 0, OP_IO::NO_RD_WR);                                          /* COMMIT; */
  const inst_def_t LABEL_INST = DEF_INST(0x01, LABEL,   "[ ]*" LABEL_REGEX "[ ]*:.*", 0, OP_IO::NO_RD_WR);                                 /* myLabel: */
  const inst_def_t CODELET_INST = DEF_INST(0x10, CODELET, "[ ]*COD[ ]+([a-zA-Z0-9_]+)[ ]+([a-zA-Z0-9_, ]*);.*", 0, OP_IO::NO_RD_WR);      /* COD codelet_name arg1, arg2, arg3; */
  
  // CONTROL FLOW INSTRUCTIONS
  /** \brief All the control instructions
   *
   * All the different regular expressions used for Control flow instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static inst_def_t const controlInsts[] = {
    DEF_INST(0x20, JMPLBL,  "[ ]*(JMPLBL)[ ]+(" LABEL_REGEX ");.*", 1, OP_IO::NO_RD_WR),                                                   /* JMPLBL destination;*/
    DEF_INST(0x21, JMPPC,   "[ ]*(JMPPC)[ ]+(" INMIDIATE_REGEX ");.*", 1, OP_IO::NO_RD_WR),                                                      /* JMPPC -100;*/
    DEF_INST(0x22, BREQ,    "[ ]*(BREQ)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" LABEL_REGEX ");.*", 3, OP_IO::OP1_RD | OP_IO::OP2_RD),            /* BREQ R1, R2, -100; */
    DEF_INST(0x23, BGT,     "[ ]*(BGT)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" LABEL_REGEX ");.*", 3, OP_IO::OP1_RD | OP_IO::OP2_RD),             /* BGT R1, R2, -100; */
    DEF_INST(0x24, BGET,    "[ ]*(BGET)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" LABEL_REGEX ");.*", 3, OP_IO::OP1_RD | OP_IO::OP2_RD),            /* BGET R1, R2, -100; */
    DEF_INST(0x25, BLT,     "[ ]*(BLT)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" LABEL_REGEX ");.*", 3, OP_IO::OP1_RD | OP_IO::OP2_RD),             /* BLT R1, R2, -100; */
    DEF_INST(0x26, BLET,    "[ ]*(BLET)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" LABEL_REGEX ");.*", 3, OP_IO::OP1_RD | OP_IO::OP2_RD)};            /* BLET R1, R2, -100; */

  #define JMPLBL_INST controlInsts[0]
  #define JMPPC_INST controlInsts[1]
  #define BREQ_INST controlInsts[2]
  #define BGT_INST controlInsts[3]
  #define BGET_INST controlInsts[4]
  #define BLT_INST controlInsts[5]
  #define BLET_INST controlInsts[6]

  //BASIC ARITHMETIC INSTRUCTIONS
  /** \brief All the basic arithmetic instructions
   *
   * All the different regular expressions used for basic arithmetic instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static inst_def_t const basicArithInsts[] = {
  DEF_INST(0x30, ADD,  "[ ]*(ADD)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3, OP_IO::OP1_WR | OP_IO::OP2_RD | OP_IO::OP3_RD),     /* ADD R1, R2, R3; R2 and R3 can be literals*/
  DEF_INST(0x31, SUB,  "[ ]*(SUB)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3, OP_IO::OP1_WR | OP_IO::OP2_RD | OP_IO::OP3_RD),     /* SUB R1, R2, R3; R2 and R3 can be literals*/
  DEF_INST(0x32, SHFL, "[ ]*(SHFL)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*;.*", 2, OP_IO::OP1_RD | OP_IO::OP1_WR | OP_IO::OP2_RD),                            /* SHFL R1, R2; R2 can be a literal representing how many positions to shift*/
  DEF_INST(0x33, SHFR, "[ ]*(SHFR)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*;.*", 2, OP_IO::OP1_RD | OP_IO::OP1_WR | OP_IO::OP2_RD),                            /* SHFR R1, R2; R2 can be a literal representing how many positions to shift*/
  DEF_INST(0x34, MULT,  "[ ]*(MULT)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3, OP_IO::OP1_WR | OP_IO::OP2_RD | OP_IO::OP3_RD)};    /* MULT R1, R2, R3; R2 and R3 can be literals*/

  #define ADD_INST basicArithInsts[0]
  #define SUB_INST basicArithInsts[1]
  #define SHFL_INST basicArithInsts[2]
  #define SHFR_INST basicArithInsts[3]
  #define MULT_INST basicArithInsts[4]

  //MEMORY INSTRUNCTIONS
  /** \brief All the memory related instructions
   *
   * All the different regular expressions used for Control flow instructions.  
   * each has its own format. This regexp can be used to identify the instruction
   * or to obtain the parameters
   */
  static inst_def_t const memInsts[] = {
  DEF_INST(0x40, LDIMM, "[ ]*(LDIMM)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX ")[ ]*;.*", 2, OP_IO::OP1_WR),                          /* LDADR R1, R2; R2 can be a literal or the address in a the register*/
  DEF_INST(0x41, LDADR, "[ ]*(LDADR)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 2, OP_IO::OP1_WR | OP_IO::OP2_RD),                          /* LDADR R1, R2; R2 can be a literal or the address in a the register*/
  DEF_INST(0x42, LDOFF, "[ ]*(LDOFF)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3, OP_IO::OP1_WR | OP_IO::OP2_RD | OP_IO::OP3_RD),  /* LDOFF R1, R2, R3; R1 is the base destination register, R2 is the base address, R3 is the offset. R2 and R3 can be literals */
  DEF_INST(0x43, STADR, "[ ]*(STADR)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 2, OP_IO::OP1_RD| OP_IO::OP2_RD),                          /* STADR R1, R2; R2 can be a literal or the address in a the register*/
  DEF_INST(0x44, STOFF, "[ ]*(STOFF)[ ]+(" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*,[ ]*(" INMIDIATE_REGEX "|" REGISTER_REGEX ")[ ]*;.*", 3, OP_IO::OP1_RD | OP_IO::OP2_RD | OP_IO::OP3_RD)};  /* STOFF R1, R2, R3; R1 is the base destination register, R2 is the base address, R3 is the offset. R2 and R3 can be literals */

  #define LDIMM_INST memInsts[0]
  #define LDADR_INST memInsts[1]
  #define LDOFF_INST memInsts[2]
  #define STADR_INST memInsts[3]
  #define STOFF_INST memInsts[4]




  /** \brief Definition of the instruction types 
   *
   *  There are several instructions types. This is important for the decoding of instructions
   *  and for knwoing where to send the instruction for execution 
   */
  enum instType {UNKNOWN, COMMIT, CONTROL_INST, BASIC_ARITH_INST, EXECUTE_INST, MEMORY_INST};
  #define instType_str(type) (\
    type == instType::UNKNOWN ? std::string("instType::UNKNOWN") : \
    type == instType::COMMIT ? std::string("instType::COMMIT"): \
    type == instType::CONTROL_INST ? std::string("instType::CONTROL_INST"): \
    type == instType::BASIC_ARITH_INST ? std::string("instType::BASIC_ARITH_INST"): \
    type == instType::EXECUTE_INST ? std::string("instType::EXECUTE_INST") : \
    type == instType::MEMORY_INST ? std::string("instType::MEMORY_INST") : \
    std::string("?????"))

  /** \brief Decoded register structure
   *  
   *  Contains the register name a register size that is taken out of a encoded register
   *
   */
  struct decoded_reg_t {
    std::string reg_name;
    std::string reg_size;
    uint32_t reg_size_bytes;
    uint32_t reg_number;
    unsigned char * reg_ptr;
    
    decoded_reg_t():
      reg_name(std::string("")),reg_size(std::string("")), reg_size_bytes(0), reg_number(0), reg_ptr(nullptr) { };

    decoded_reg_t(std::string regName, std::string sizeStr, uint32_t sizeBytes, uint32_t regNum, unsigned char * ptr):
      reg_name(regName), reg_size(sizeStr), reg_size_bytes(sizeBytes), reg_number(regNum), reg_ptr(ptr) { };

    decoded_reg_t(const decoded_reg_t & other) {
      reg_name = other.reg_name;
      reg_size = other.reg_size;
      reg_size_bytes = other.reg_size_bytes;
      reg_number = other.reg_number;
      reg_ptr = other.reg_ptr;
    }

    void operator=(decoded_reg_t const & other) {
      reg_name = other.reg_name;
      reg_size = other.reg_size;
      reg_size_bytes = other.reg_size_bytes;
      reg_number = other.reg_number;
      reg_ptr = other.reg_ptr;
    }

    bool operator==(decoded_reg_t const & other) const {
      return other.reg_ptr == this->reg_ptr;
    }
    bool operator!=(decoded_reg_t const & other) const {
      return other.reg_ptr != this->reg_ptr;
    }

    bool operator<(decoded_reg_t const & other) const {
      return this->reg_ptr < other.reg_ptr ;
    }
  };

  /** \brief Decoded operand structure
   *  
   *  An operand can be either a register or an immediate value, it is enconded in a union and it gets used accordingly 
   *  depending on the type.
   *   
   *
   */
  struct operand_t {
    enum {UNKNOWN, REGISTER, IMMEDIATE_VAL, LABEL} type;
  #define operand_t_str(type) (\
    type == operand_t::UNKNOWN ? std::string("operand_t::UNKNOWN") : \
    type == operand_t::REGISTER ? std::string("operand_t::REGISTER"): \
    type == operand_t::IMMEDIATE_VAL ? std::string("operand_t::IMMEDIATE_VAL"): \
    type == operand_t::LABEL ? std::string("operand_t::LABEL"): \
    std::string("?????"))
    union value_t {
      uint64_t immediate;
      decoded_reg_t reg;
      value_t (): reg(){}
      ~value_t() {};
    };
    value_t value;
    bool read;
    bool write;
    bool full_empty; // Used by ILP to determine if dependency is satisfied or not true=full and false=empty

    operand_t() : read(false), write(false), full_empty(false) {
      type = UNKNOWN;
    }

    operand_t(operand_t const & other) {
      type = other.type;
      if (type == REGISTER)
        value.reg = other.value.reg;
      else if (type == IMMEDIATE_VAL || type == LABEL)
        value.immediate = other.value.immediate;
      read = other.read;
      write = other.write;
      full_empty = other.full_empty;
    }

    operand_t(operand_t && other) {
      type = other.type;
      if (type == REGISTER)
        value.reg = other.value.reg;
      else if (type == IMMEDIATE_VAL || type == LABEL)
        value.immediate = other.value.immediate;
      read = other.read;
      write = other.write;
      full_empty = other.full_empty;
    }

    void operator=(operand_t const & other) {
      type = other.type;
      if (type == REGISTER)
        value.reg = other.value.reg;
      else if (type == IMMEDIATE_VAL || type == LABEL)
        value.immediate = other.value.immediate;
      read = other.read;
      write = other.write;
      full_empty = other.full_empty;
    }
  };
}

namespace std {
    template <>
        class hash<scm::decoded_reg_t>{
        public :
            size_t operator()(const scm::decoded_reg_t &reg ) const
            {
                return hash<unsigned char *>()(reg.reg_ptr);
            }
    };
};


#endif