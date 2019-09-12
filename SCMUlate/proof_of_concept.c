#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "SCMUlate_tools.h"
#include "machine_configuration.h"

// FUNCTION DEFINITIONS
int SCMUlate ();
int SCMUlate_SU_Behavior();
int SCMUlate_MEM_Behavior();
int SCMUlate_CU_Behavior();

int main () {
    // === INITIALIZATION ====
    // We check the register configuration is valid
    if(!checkRegisterConfig()) {
        return 1;
    }
    describeRegisterFile();

    // === EMULATION STARTING ===
    SCMULATE_ERROR_IF(0, SCMUlate() != 0, "ERROR EXECUTING SCMUlate() function");
    return 0;
}

int SCMUlate () {

#pragma omp parallel
    {
        #pragma omp master 
        {
            SCMULATE_INFOMSG(1, "Running with %d threads ", omp_get_num_threads());
        }
        switch (omp_get_thread_num()) {
            case SU_THREAD:
                SCMUlate_SU_Behavior();
                break;
            case MEM_THREAD:
                SCMUlate_MEM_Behavior();
                break;
            case CU_THREADS:
                SCMUlate_CU_Behavior();
                break;
            default:
                SCMULATE_WARNING(0, "Thread created with no purpose. What's my purpose? You pass the butter");
        }

    }

    return 0;
}

int SCMUlate_SU_Behavior() {
    SCMULATE_INFOMSG(1, "I am an SU")
    return 0;
}
int SCMUlate_MEM_Behavior() {
    SCMULATE_INFOMSG(1, "I am MEM")
    return 0;
}
int SCMUlate_CU_Behavior() {
    SCMULATE_INFOMSG(1, "I am an CU")
    return 0;
}