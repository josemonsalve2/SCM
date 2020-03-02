#ifndef __REGISTER_CONFIG__
#define __REGISTER_CONFIG__

// Size of L3 in KB
#define REG_FILE_SIZE_KB 12288l
// Cache line in bytes
#define CACHE_LINE_SIZE 64l

// Number of registers created
#define NUM_REG_64BITS 160l
#define NUM_REG_1LINE 140l
#define NUM_REG_8LINE 100l
#define NUM_REG_16LINE 100l
#define NUM_REG_256LINE 60l
#define NUM_REG_512LINE 60l
#define NUM_REG_1024LINE 60l
#define NUM_REG_2048LINE 40l

#define CALCULATE_REG_SIZE (NUM_REG_64BITS*8l + \
    (NUM_REG_1LINE + \
     NUM_REG_8LINE*8l + \
     NUM_REG_16LINE*16l + \
     NUM_REG_256LINE*256l + \
     NUM_REG_512LINE*512l + \
     NUM_REG_1024LINE*1024l + \
     NUM_REG_2048LINE*2048l)*CACHE_LINE_SIZE)

namespace scm {

  typedef struct{
      unsigned char data[8];
  } _reg_64bits;
  typedef struct{
      unsigned char data[CACHE_LINE_SIZE];
  } _reg_1line;
  typedef struct{
      unsigned char data[CACHE_LINE_SIZE*8];
  } _reg_8line;
  typedef struct{
      unsigned char data[CACHE_LINE_SIZE*16];
  } _reg_16line;
  typedef struct{
      unsigned char data[CACHE_LINE_SIZE*256];
  } _reg_256line;
  typedef struct{
      unsigned char data[CACHE_LINE_SIZE*512];
  } _reg_512line;
  typedef struct{
      unsigned char data[CACHE_LINE_SIZE*1024];
  } _reg_1024line;
  typedef struct{
      unsigned char data[CACHE_LINE_SIZE*2048];
  } _reg_2048line;
  
  typedef struct {
  #if NUM_REG_64BITS != 0
          _reg_64bits reg_64[NUM_REG_64BITS];
  #endif
  #if NUM_REG_1LINE != 0
          _reg_1line reg_1l[NUM_REG_1LINE];
  #endif
  #if NUM_REG_8LINE != 0
          _reg_8line reg_8l[NUM_REG_8LINE];
  #endif
  #if NUM_REG_16LINE != 0
          _reg_16line reg_16l[NUM_REG_16LINE];
  #endif
  #if NUM_REG_256LINE != 0
          _reg_256line reg_256l[NUM_REG_256LINE];
  #endif
  #if NUM_REG_512LINE != 0
          _reg_512line reg_512l[NUM_REG_512LINE];
  #endif
  #if NUM_REG_1024LINE != 0
          _reg_1024line reg_1024l[NUM_REG_1024LINE];
  #endif
  #if NUM_REG_2048LINE != 0
          _reg_2048line reg_2048l[NUM_REG_2048LINE];
  #endif
      } registers_t;
}
#define REG_64B(num) (reg_file->registers.reg_64[num])
#define REG_1L(num) (reg_file->registers.reg_1l[num])
#define REG_8L(num) (reg_file->registers.reg_8l[num])
#define REG_16L(num) (reg_file->registers.reg_16l[num])
#define REG_256L(num) (reg_file->registers.reg_256l[num])
#define REG_512L(num) (reg_file->registers.reg_512l[num])
#define REG_1024L(num) (reg_file->registers.reg_1024l[num])
#define REG_2048L(num) (reg_file->registers.reg_2048l[num])

#endif
