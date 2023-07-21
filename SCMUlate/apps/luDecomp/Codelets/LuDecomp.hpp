#ifndef __COD_LUDECOMP__
#define __COD_LUDECOMP__

#include "codelet.hpp"
// relatively sure these are the default values used in bots
#define bots_arg_size 50
#define bots_arg_size_1 100
#define TRUE 1
#define FALSE 0

extern float ** BENCH;

DEFINE_CODELET(fwd_2048L, 2, scm::OP_IO::OP1_RD | scm::OP_IO::OP1_WR | scm::OP_IO::OP2_RD);

DEFINE_CODELET(bdiv_2048L, 2, scm::OP_IO::OP1_RD | scm::OP_IO::OP1_WR | scm::OP_IO::OP2_RD);

DEFINE_CODELET(bmod_2048L, 3, scm::OP_IO::OP1_RD | scm::OP_IO::OP1_WR | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD);

DEFINE_CODELET(lu0_2048L, 1, scm::OP_IO::OP1_RD | scm::OP_IO::OP1_WR)

DEFINE_CODELET(zero_2048L, 1, scm::OP_IO::OP1_WR);

// now bench is externed and we will not take the address as a register
// loads a submat into register (OP1) -- takes row offset (OP2) and column offset (OP3)
DEFINE_MEMORY_CODELET(loadSubMat_2048L, 3, scm::OP_IO::OP1_WR | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD, 0);

DEFINE_MEMORY_CODELET(storeSubMat_2048L, 3, scm::OP_IO::OP1_RD | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD, 0);

DEFINE_MEMORY_CODELET(loadBenchAddr_64B, 1, scm::OP_IO::OP1_WR, 0);

#endif
