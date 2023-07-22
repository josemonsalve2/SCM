#ifndef __COD_MATMUL__
#define __COD_MATMUL__

#include "codelet.hpp"

// C += AxB Where A, B, and C are 128x128 square matrices.
DEFINE_CODELET(Sleep_2048L, 1, scm::OP_IO::OP1_RD);

#endif
