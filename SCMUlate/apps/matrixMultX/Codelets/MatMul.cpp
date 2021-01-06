#include "MatMul.hpp"
#include <iomanip>
#ifdef BLAS
#include <cblas.h>
#endif
#ifdef MKL
#include <mkl.h>
#endif

#define TILE_DIM 128

MEMRANGE_CODELET(LoadSqTile_2048L, 
  // Obtaining the parameters
  unsigned char *reg2 = this->getParams().getParamAs(1); // Getting register 2
  unsigned char *reg3 = this->getParams().getParamAs(2); // Getting register 3
  uint64_t address = reinterpret_cast<uint8_t*>(reg2)[0];
    uint64_t ldistance = reinterpret_cast<uint8_t*>(reg3)[0];
  for (int i = 1; i < 8; i++) {
    address <<= 8;
    address += static_cast<uint8_t>(reg2[i]);
    ldistance <<= 8;
    ldistance += static_cast<uint8_t>(reg3[i]);
  }
  // Add the ranges
  for (uint64_t i = 0; i < TILE_DIM; i++) {
    this->addMemRange(address+ldistance*sizeof(double)*i, TILE_DIM*sizeof(double));
  }
);

IMPLEMENT_CODELET(LoadSqTile_2048L,
  unsigned char *reg1 = this->getParams().getParamAs(0); // Getting register 1
  double *destReg = reinterpret_cast<double*>(reg1);
  int i = 0;
  for (auto it = memoryRanges->begin(); it != memoryRanges->end(); it++) {
    double *addressStart = reinterpret_cast<double *> (getAddress(it->memoryAddress)); // Address L2 memory to a pointer of the runtime
    std::memcpy(destReg+TILE_DIM*i++, addressStart, it->size);
  }
);

IMPLEMENT_CODELET(MatMult_2048L,
  // Obtaining the parameters
  unsigned char *reg1 = this->getParams().getParamAs(0); // Getting register 1
  unsigned char *reg2 = this->getParams().getParamAs(1); // Getting register 2
  unsigned char *reg3 = this->getParams().getParamAs(2); // Getting register 3
  double *A = reinterpret_cast<double*>(reg2);
  double *B = reinterpret_cast<double*>(reg3);
  double *C = reinterpret_cast<double*>(reg1);

  // for (int i = 0; i < TILE_DIM; i++)
  //   for (int j = 0; j < TILE_DIM; j++)
  //     for (int k = 0; k < TILE_DIM; k++)
        // C[i + j*TILE_DIM] = A[i + k*TILE_DIM]*B[j*TILE_DIM + k]; 
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, TILE_DIM, TILE_DIM, TILE_DIM, 1, A, TILE_DIM, B, TILE_DIM, 1, C, TILE_DIM);

);

// IMPLEMENT_CODELET(MatMultReduc_2048L,
//   // Obtaining the parameters
//   unsigned char *reg1 = this->getParams().getParamAs(0); // Getting register 1
//   unsigned char *reg2 = this->getParams().getParamAs(1); // Getting register 2
//   unsigned char *reg3 = this->getParams().getParamAs(2); // Getting register 3
//   double *A = reinterpret_cast<double*>(reg2);
//   double *B = reinterpret_cast<double*>(reg3);
//   double *C = reinterpret_cast<double*>(reg1);

//   for (int i = 0; i < TILE_DIM*TILE_DIM; i++)
//     C[i] += A[i] + B[i];

// );

MEMRANGE_CODELET(StoreSqTile_2048L, 
  // Obtaining the parameters
  unsigned char *reg2 = this->getParams().getParamAs(1); // Getting register 2
  unsigned char *reg3 = this->getParams().getParamAs(2); // Getting register 3
  uint64_t address = reinterpret_cast<uint8_t*>(reg2)[0];
  uint64_t ldistance = reinterpret_cast<uint8_t*>(reg3)[0];

  for (int i = 1; i < 8; i++) {
    address <<= 8;
    address += static_cast<uint8_t>(reg2[i]);
    ldistance <<= 8;
    ldistance += static_cast<uint8_t>(reg3[i]);
  }
  for (uint64_t i = 0; i < TILE_DIM; i++) {
    this->addMemRange(address+ldistance*sizeof(double)*i, TILE_DIM*sizeof(double));
  }
);

IMPLEMENT_CODELET(StoreSqTile_2048L,
  // Obtaining the parameters
  unsigned char *reg1 = this->getParams().getParamAs(0); // Getting register 1
  double *sourceReg = reinterpret_cast<double*>(reg1);

  int i = 0;
  for (auto it = memoryRanges->begin(); it != memoryRanges->end(); it++) {
    double *addressStart = reinterpret_cast<double *> (getAddress(it->memoryAddress)); // Address L2 memory to a pointer of the runtime
    std::memcpy(addressStart, sourceReg+TILE_DIM*i++, it->size);
  }
);
