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
  uint8_t* address_reg = this->getParams().getParamValueAs<uint8_t*>(2);
  uint64_t ldistance = this->getParams().getParamValueAs<uint64_t>(3);

  uint64_t address = address_reg[0];
  ldistance *= sizeof(double);
  for (int i = 1; i < 8; i++) {
    address <<= 8;
    address += address_reg[i];
  }
  // Add the ranges
  for (uint64_t i = 0; i < TILE_DIM; i++) {
    this->addReadMemRange(address+ldistance*i, TILE_DIM*sizeof(double));
  }
);
IMPLEMENT_CODELET(LoadSqTile_2048L,
  double *destReg = this->getParams().getParamValueAs<double*>(1);
  int i = 0;
  for (auto it = memoryRanges->reads.begin(); it != memoryRanges->reads.end(); it++) {
    double *addressStart = reinterpret_cast<double *> (getAddress(it->memoryAddress)); // Address L2 memory to a pointer of the runtime
    std::memcpy(destReg+TILE_DIM*i++, addressStart, it->size);
  }
);

IMPLEMENT_CODELET(MatMult_2048L,
  double *A = this->getParams().getParamValueAs<double*>(2);
  double *B = this->getParams().getParamValueAs<double*>(3);
  double *C = this->getParams().getParamValueAs<double*>(1);

  // for (int i = 0; i < TILE_DIM; i++)
  //   for (int j = 0; j < TILE_DIM; j++)
  //     for (int k = 0; k < TILE_DIM; k++)
        // C[i + j*TILE_DIM] = A[i + k*TILE_DIM]*B[j*TILE_DIM + k]; 
#ifndef NOBLAS
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, TILE_DIM, TILE_DIM, TILE_DIM, 
              1, A, TILE_DIM, B, TILE_DIM, 1, C, TILE_DIM);
#else
  for (int i=0; i<TILE_DIM; i=i+1){
    for (int j=0; j<TILE_DIM; j=j+1){
        for (int k=0; k<TILE_DIM; k=k+1) {
          C[i*TILE_DIM + j]+=((A[i*TILE_DIM + k])*(B[k*TILE_DIM + j]));
        }
    }
   }
#endif
);

MEMRANGE_CODELET(StoreSqTile_2048L, 
  // Obtaining the parameters
  uint8_t* address_reg = this->getParams().getParamValueAs<uint8_t*>(2);
  uint64_t ldistance = this->getParams().getParamValueAs<uint64_t>(3);

  uint64_t address = address_reg[0];
  ldistance *= sizeof(double);
  for (int i = 1; i < 8; i++) {
    address <<= 8;
    address += address_reg[i];
  }
  for (uint64_t i = 0; i < TILE_DIM; i++) {
    this->addWriteMemRange(address+ldistance*i, TILE_DIM*sizeof(double));
  }
);

IMPLEMENT_CODELET(StoreSqTile_2048L,
  // Obtaining the parameters
  double *sourceReg = this->getParams().getParamValueAs<double*>(1);

  int i = 0;
  for (auto it = memoryRanges->writes.begin(); it != memoryRanges->writes.end(); it++) {
    double *addressStart = reinterpret_cast<double *> (getAddress(it->memoryAddress)); // Address L2 memory to a pointer of the runtime
    std::memcpy(addressStart, sourceReg+TILE_DIM*i++, it->size);
  }
);
