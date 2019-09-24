#ifndef __FETCH_DECODE__
#define __FETCH_DECODE__

#include "SCMUlate_tools.h"
#include "register_configuration.h"


typedef struct instruction_t {
    char op_name[20];
    int op_code;
};

extern int alive;
int SCMUlate_SU_Behavior();
#endif
