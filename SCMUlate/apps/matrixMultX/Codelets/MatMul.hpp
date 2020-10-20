#ifndef __COD_MATMUL__
#define __COD_MATMUL__

#include "codelet.hpp"

// [Rx_2048L, address, padding] = Load from addr 128x128 elements to register with offset padding  
// Loads tile of 128x128 
DEFINE_CODELET(LoadSqTile_2048L, 3, scm::OP_IO::OP1_WR | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD); 

// C += AxB Where A, B, and C are 128x128 square matrices.
DEFINE_CODELET(MatMult_2048L, 3, scm::OP_IO::OP1_WR | scm::OP_IO::OP1_RD | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD); 

// [Rx_2048L, address, padding] = Store to addr 128x128 elements from register with offset padding  
// Loads tile of 128x128 
DEFINE_CODELET(StoreSqTile_2048L, 3, scm::OP_IO::OP1_WR | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD); 

#endif
