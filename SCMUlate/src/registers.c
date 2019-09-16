#include "register_configuration.h"
#include "SCMUlate_tools.h"

register_file_t reg_file;


void describeRegisterFile() {

    SCMULATE_INFOMSG(0, "REGISTER FILE DEFINITION");
    SCMULATE_INFOMSG(0, " SIZE = %ld", CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(1, " %ld registers of 64BITS, each of %d bytes. Total size = %ld  -- %f percent ", NUM_REG_64BITS, 64/8, NUM_REG_64BITS*64/8, (NUM_REG_64BITS*64/8)*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(1, " %ld registers of 1LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_1LINE, CACHE_LINE_SIZE, NUM_REG_1LINE*CACHE_LINE_SIZE, NUM_REG_1LINE*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(1, " %ld registers of 8LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_8LINE, CACHE_LINE_SIZE*8, NUM_REG_8LINE*8*CACHE_LINE_SIZE, NUM_REG_8LINE*8*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(1, " %ld registers of 16LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_16LINE, CACHE_LINE_SIZE*16, NUM_REG_16LINE*16*CACHE_LINE_SIZE, NUM_REG_16LINE*16*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(1, " %ld registers of 256LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_256LINE, CACHE_LINE_SIZE*256, NUM_REG_256LINE*256*CACHE_LINE_SIZE, NUM_REG_256LINE*256*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(1, " %ld registers of 512LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_512LINE, CACHE_LINE_SIZE*512, NUM_REG_512LINE*512*CACHE_LINE_SIZE, NUM_REG_512LINE*512*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(1, " %ld registers of 1024LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_1024LINE, CACHE_LINE_SIZE*1024, NUM_REG_1024LINE*1024*CACHE_LINE_SIZE, NUM_REG_1024LINE*1024*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);
    SCMULATE_INFOMSG(1, " %ld registers of 2048LINE each of %ld bytes. Total size = %ld  -- %f percent", NUM_REG_2048LINE, CACHE_LINE_SIZE*2048, NUM_REG_2048LINE*2048*CACHE_LINE_SIZE, NUM_REG_2048LINE*2048*CACHE_LINE_SIZE*100.0f/CALCULATE_REG_SIZE);

}


int checkRegisterConfig() {
    if (CALCULATE_REG_SIZE > REG_FILE_SIZE*1000) {
        // This is an error, because it could cause seg fault
        SCMULATE_ERROR(0, "DEFINED REGISTER IS LARGER THAN DEFINED REG_FILE_SIZE");
        SCMULATE_ERROR(0, "REG_FILE_SIZE = %ld", REG_FILE_SIZE*1000l);
        SCMULATE_ERROR(0, "CALCULATE_REG_SIZE = %ld", CALCULATE_REG_SIZE);
        SCMULATE_ERROR(0, "EXCESS = %ld", CALCULATE_REG_SIZE - REG_FILE_SIZE*1000l);
        return 0;
    } else if (CALCULATE_REG_SIZE < REG_FILE_SIZE*1000) {
        // This is just a warning, you are not using the whole register file
        SCMULATE_WARNING(0, "DEFINED REGISTER IS SMALLER THAN DEFINED REG_FILE_SIZE");
        SCMULATE_WARNING(0, "REG_FILE_SIZE = %ld", REG_FILE_SIZE*1000l);
        SCMULATE_WARNING(0, "CALCULATE_REG_SIZE = %ld", CALCULATE_REG_SIZE);
        SCMULATE_WARNING(0, "REMAINING = %ld", (REG_FILE_SIZE*1000l) - CALCULATE_REG_SIZE);
    }

    return 1;
}

char * getRegister(char * size, int num) {
    char * result = NULL;
    if (strcmp(size, "64B") == 0){
        result = REG_64B(num).data;
    } else if (strcmp(size,"1L") ==0 ) {
        result = REG_1L(num).data;
    } else if (strcmp(size,"8L") ==0 ) {
        result = REG_8L(num).data;
    } else if (strcmp(size,"16L") ==0 ) {
        result = REG_16L(num).data;
    } else if (strcmp(size,"256L") ==0 ) {
        result = REG_256L(num).data;
    } else if (strcmp(size,"512L") ==0 ) {
        result = REG_512L(num).data;
    } else if (strcmp(size,"1024L") ==0 ) {
        result = REG_1024L(num).data;
    } else if (strcmp(size,"2048L") ==0 ) {
        result = REG_2048L(num).data;
    } else {
        SCMULATE_ERROR(0, "DECODED REGISTER DOES NOT EXIST!!!")
    }
    return result;
}
