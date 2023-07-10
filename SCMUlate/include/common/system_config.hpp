#ifndef __SCMULATE_SYS_CONFIG__
#define __SCMULATE_SYS_CONFIG__
#define RESERVATION_TABLE_SIZE 100

namespace scm {
    enum ILP_MODES {SEQUENTIAL, SUPERSCALAR, OOO};
    const char *const ILP_MODES_STR[] = {
        "ILP_MODES::SEQUENTIAL", "ILP_MODES::SUPERSCALAR", "ILP_MODES::OOO"};
    enum DUPL_MODES {
      NO_DUPLICATION,
      TWO_OUT_OF_THREE,
      THREE_OUT_OF_FIVE,
      ADAPTIVE_DUPLICATION
    };
    const char *const DUPL_MODES_STR[] = {
        "DUPL_MODES::NO_DUPLICATION", "DUPL_MODES::TWO_OUT_OF_THREE",
        "DUPL_MODES::THREE_OUT_OF_FIVE", "DUPL_MODES::ADAPTIVE_DUPLICATION"};
}

// This cannot be freely changed. it still has
// some hardcoded code in the program
#define MAX_NUM_OPERANDS 3

#define INSTRUCTIONS_BUFFER_SIZE 128
#define EXECUTION_QUEUE_SIZE 1
#define INSTRUCTION_FETCH_WINDOW 2
#ifndef DEBUGER_MODE
#define DEBUGER_MODE 0
#endif

#endif // __SCMULATE_SYS_CONFIG__