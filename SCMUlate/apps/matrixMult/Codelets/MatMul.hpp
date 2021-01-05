#ifndef __COD_MATMUL__
#define __COD_MATMUL__

#include "codelet.hpp"

// [Rx_2048L, address, padding] = Load from addr 128x128 elements to register with offset padding  
// Loads tile of 128x128 
DEFINE_MEMORY_CODELET(LoadSqTile_2048L, 3, scm::OP_IO::OP1_WR | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD, scm::OP_ADDRESS::OP2_IS_ADDRESS | scm::OP_ADDRESS::OP3_IS_ADDRESS); 

// C += AxB Where A, B, and C are 128x128 square matrices.
DEFINE_CODELET(MatMult_2048L, 3, scm::OP_IO::OP1_WR | scm::OP_IO::OP1_RD | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD); 

// [Rx_2048L, address, padding] = Store to addr 128x128 elements from register with offset padding  
// Loads tile of 128x128 
DEFINE_MEMORY_CODELET(StoreSqTile_2048L, 3, scm::OP_IO::OP1_RD | scm::OP_IO::OP2_RD | scm::OP_IO::OP3_RD, scm::OP_ADDRESS::OP2_IS_ADDRESS | scm::OP_ADDRESS::OP3_IS_ADDRESS); 

#endif
