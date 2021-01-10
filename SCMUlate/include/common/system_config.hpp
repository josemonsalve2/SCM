#ifndef __SCMULATE_SYS_CONFIG__
#define __SCMULATE_SYS_CONFIG__
#define RESERVATION_TABLE_SIZE 100

namespace scm {
    enum ILP_MODES {SEQUENTIAL, SUPERSCALAR, OOO};
}

#define INSTRUCTIONS_BUFFER_SIZE 128
#define EXECUTION_QUEUE_SIZE 2
#define INSTRUCTION_FETCH_WINDOW 2
#ifndef DEBUGER_MODE
#define DEBUGER_MODE 0
#endif

#endif // __SCMULATE_SYS_CONFIG__