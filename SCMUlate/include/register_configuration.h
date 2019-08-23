// Size of L3 in KB
#define REG_FILE_SIZE 12288l
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

typedef struct{
    char data[8];
} _reg_64bits;
typedef struct{
    char data[CACHE_LINE_SIZE];
} _reg_1line;
typedef struct{
    char data[CACHE_LINE_SIZE*8];
} _reg_8line;
typedef struct{
    char data[CACHE_LINE_SIZE*16];
} _reg_16line;
typedef struct{
    char data[CACHE_LINE_SIZE*256];
} _reg_256line;
typedef struct{
    char data[CACHE_LINE_SIZE*512];
} _reg_512line;
typedef struct{
    char data[CACHE_LINE_SIZE*1024];
} _reg_1024line;
typedef struct{
    char data[CACHE_LINE_SIZE*2048];
} _reg_2048line;

typedef union register_t{
    char space[REG_FILE_SIZE*1000];
    struct {
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
    }registers;
} reg_file;


#define REG_64B(num) (ref_file.registers.reg_64[num])
#define REG_1L(num) (ref_file.registers.reg_1l[num])
#define REG_8L(num) (ref_file.registers.reg_8l[num])
#define REG_16L(num) (ref_file.registers.reg_16l[num])
#define REG_256L(num) (ref_file.registers.reg_256l[num])
#define REG_512L(num) (ref_file.registers.reg_512l[num])
#define REG_1024L(num) (ref_file.registers.reg_1024l[num])
#define REG_2048L(num) (ref_file.registers.reg_2048l[num])

void describeRegisterFile() {

    SCMULATE_INFOMSG("REGISTER FILE DEFINITION");
    SCMULATE_INFOMSG(" SIZE = %ld", CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(" %ld registers of 64BITS, each of %d bytes. Total size = %ld  -- %f percent ", NUM_REG_64BITS, 64/8, NUM_REG_64BITS*64/8, (NUM_REG_64BITS*64/8)*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(" %ld registers of 1LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_1LINE, CACHE_LINE_SIZE, NUM_REG_1LINE*CACHE_LINE_SIZE, NUM_REG_1LINE*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(" %ld registers of 8LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_8LINE, CACHE_LINE_SIZE*8, NUM_REG_8LINE*8*CACHE_LINE_SIZE, NUM_REG_8LINE*8*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(" %ld registers of 16LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_16LINE, CACHE_LINE_SIZE*16, NUM_REG_16LINE*16*CACHE_LINE_SIZE, NUM_REG_16LINE*16*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(" %ld registers of 256LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_256LINE, CACHE_LINE_SIZE*256, NUM_REG_256LINE*256*CACHE_LINE_SIZE, NUM_REG_256LINE*256*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(" %ld registers of 512LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_512LINE, CACHE_LINE_SIZE*512, NUM_REG_512LINE*512*CACHE_LINE_SIZE, NUM_REG_512LINE*512*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(" %ld registers of 1024LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_1024LINE, CACHE_LINE_SIZE*1024, NUM_REG_1024LINE*1024*CACHE_LINE_SIZE, NUM_REG_1024LINE*1024*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(" %ld registers of 2048LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_2048LINE, CACHE_LINE_SIZE*2048, NUM_REG_2048LINE*2048*CACHE_LINE_SIZE, NUM_REG_2048LINE*2048*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);

}


int checkRegisterConfig() {
    if (CALCULATE_REG_SIZE > REG_FILE_SIZE*1000) {
        // This is an error, because it could cause seg fault
        SCMULATE_ERROR("DEFINED REGISTER IS LARGER THAN DEFINED REG_FILE_SIZE");
        SCMULATE_ERROR("REG_FILE_SIZE = %ld", REG_FILE_SIZE*1000l);
        SCMULATE_ERROR("CALCULATE_REG_SIZE = %ld", CALCULATE_REG_SIZE);
        SCMULATE_ERROR("EXCESS = %ld", CALCULATE_REG_SIZE - REG_FILE_SIZE*1000l);
        return 0;
    } else if (CALCULATE_REG_SIZE < REG_FILE_SIZE*1000) {
        // This is just a warning, you are not using the whole register file
        SCMULATE_WARNING("DEFINED REGISTER IS SMALLER THAN DEFINED REG_FILE_SIZE");
        SCMULATE_WARNING("REG_FILE_SIZE = %ld", REG_FILE_SIZE*1000l);
        SCMULATE_WARNING("CALCULATE_REG_SIZE = %ld", CALCULATE_REG_SIZE);
        SCMULATE_WARNING("REMAINING = %ld", (REG_FILE_SIZE*1000l) - CALCULATE_REG_SIZE);
    }

    return 1;
}