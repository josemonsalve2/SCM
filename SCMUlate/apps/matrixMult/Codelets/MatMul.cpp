#include "MatMul.hpp"
#include <iomanip>
#ifdef BLAS
#include <blas.h>
#endif
#ifdef MKL
#include <mkl.h>
#endif

#define TILE_DIM 128
// JOSE  TOOD: We need a mechanism that allows us  to translatee  a memory  address  
IMPLEMENT_CODELET(LoadSqTile_2048L,
  // Obtaining the parameters
  unsigned char ** args = static_cast<unsigned char **>(this->getParams());
  unsigned char *reg1 = args[0]; // Getting register 1
  unsigned char *reg2 = args[1]; // Getting register 2
  unsigned char *reg3 = args[2]; // Getting register 3
  double *destReg = reinterpret_cast<double*>(reg1);
  double *addressStart = reinterpret_cast<double *> (getAddress(*reinterpret_cast<uint64_t*>(reg2))); // Address L2 memory to a pointer of the runtime
  uint64_t padding = *(reinterpret_cast<uint64_t*>(reg3));

  for (uint64_t i = 0; i < TILE_DIM; i++) {
    std::memcpy(destReg+TILE_DIM*i, addressStart+padding*i, TILE_DIM);
  }
);

IMPLEMENT_CODELET(MatMult_2048L,
  // Obtaining the parameters
  unsigned char ** args = static_cast<unsigned char **>(this->getParams());
  unsigned char *reg1 = args[0]; // Getting register 1
  unsigned char *reg2 = args[1]; // Getting register 2
  unsigned char *reg3 = args[2]; // Getting register 3
  double *A = reinterpret_cast<double*>(reg2);
  double *B = reinterpret_cast<double*>(reg3);
  double *C = reinterpret_cast<double*>(reg1);

  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, TILE_DIM, TILE_DIM, TILE_DIM, 1, A, 0, B, 0, 1, C, 0);

);


IMPLEMENT_CODELET(StoreSqTile_2048L,
  // Obtaining the parameters
  unsigned char ** args = static_cast<unsigned char **>(this->getParams());
  unsigned char *reg1 = args[0]; // Getting register 1
  unsigned char *reg2 = args[1]; // Getting register 2
  unsigned char *reg3 = args[2]; // Getting register 3
  double *destReg = reinterpret_cast<double*>(reg1);
  double *addressStart = reinterpret_cast<double *> (getAddress(*reinterpret_cast<uint64_t*>(reg2))); // Address L2 memory to a pointer of the runtime
  uint64_t padding = *(reinterpret_cast<uint64_t*>(reg3));

  for (uint64_t i = 0; i < TILE_DIM; i++) {
    std::memcpy(destReg+TILE_DIM*i, addressStart+padding*i, TILE_DIM);
  }
);