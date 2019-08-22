#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "SCMUlate_tools.h"
#include "machine_configuration.h"

int main () {
    // We check the register configuration is valid
    if(!checkRegisterConfig()) {
        return 1;
    }
    describeRegisterFile();

    return 0;
}

int SCMUlate () {

#pragma omp parallel
    {
        switch (omp_get_thread_num()) {
            default:
                SCMULATE_WARNING("Threa created with no purpose. What's my purpose? You pass the butter");
        }

    }

    return 0;
}



