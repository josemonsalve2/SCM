#include "MatMulOMPoffload.hpp"
#include <iomanip>

#ifdef MKL
#include "mkl.h"
#include "mkl_cblas.h"
#include "mkl_omp_offload.h"

#pragma omp requires unified_shared_memory

#define TILE_DIM 128

MEMRANGE_CODELET(LoadSqTileGPU_2048L, 
  // Obtaining the parameters
  unsigned char *reg2 = this->getParams().getParamAs(2); // Getting register 2
  unsigned char *reg3 = this->getParams().getParamAs(3); // Getting register 3
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
    this->addReadMemRange(address+ldistance*sizeof(double)*i, TILE_DIM*sizeof(double));
  }
);

IMPLEMENT_CODELET(LoadSqTileGPU_2048L,
  unsigned char *reg1 = this->getParams().getParamAs(1); // Getting register 1
  double *destReg = reinterpret_cast<double*>(reg1);
  int i = 0;
for (auto it = memoryRanges->reads.begin(); it != memoryRanges->reads.end(); it++) {
    double *addressStart = reinterpret_cast<double *> (getAddress(it->memoryAddress)); // Address L2 memory to a pointer of the runtime
    std::memcpy(destReg+TILE_DIM*i++, addressStart, it->size);
  }
);

IMPLEMENT_CODELET(MatMultGPU_2048L,
  // Obtaining the parameters
  unsigned char *reg1 = this->getParams().getParamAs(1); // Getting register 1
  unsigned char *reg2 = this->getParams().getParamAs(2); // Getting register 2
  unsigned char *reg3 = this->getParams().getParamAs(3); // Getting register 3
  double *A = reinterpret_cast<double*>(reg2);
  double *B = reinterpret_cast<double*>(reg3);
  double *C = reinterpret_cast<double*>(reg1);

    
_Pragma("omp target data map(to:A[0:TILE_DIM*TILE_DIM],B[0:TILE_DIM*TILE_DIM]) map(tofrom:C[0:TILE_DIM*TILE_DIM])")
    {

        // run gemm on gpu, use standard oneMKL interface within a variant dispatch construct
_Pragma("omp target variant dispatch use_device_ptr(A, B, C)")
        {
          cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, TILE_DIM, TILE_DIM, TILE_DIM, 1, A, TILE_DIM, B, TILE_DIM, 1, C, TILE_DIM);
        }
    }

);

MEMRANGE_CODELET(StoreSqTileGPU_2048L, 
  // Obtaining the parameters
  unsigned char *reg2 = this->getParams().getParamAs(2); // Getting register 2
  unsigned char *reg3 = this->getParams().getParamAs(3); // Getting register 3
  uint64_t address = reinterpret_cast<uint8_t*>(reg2)[0];
  uint64_t ldistance = reinterpret_cast<uint8_t*>(reg3)[0];

  for (int i = 1; i < 8; i++) {
    address <<= 8;
    address += static_cast<uint8_t>(reg2[i]);
    ldistance <<= 8;
    ldistance += static_cast<uint8_t>(reg3[i]);
  }
  for (uint64_t i = 0; i < TILE_DIM; i++) {
    this->addWriteMemRange(address+ldistance*sizeof(double)*i, TILE_DIM*sizeof(double));
  }
);

IMPLEMENT_CODELET(StoreSqTileGPU_2048L,
  // Obtaining the parameters
  unsigned char *reg1 = this->getParams().getParamAs(1); // Getting register 1
  double *sourceReg = reinterpret_cast<double*>(reg1);

  int i = 0;
  for (auto it = memoryRanges->writes.begin(); it != memoryRanges->writes.end(); it++) {
    double *addressStart = reinterpret_cast<double *> (getAddress(it->memoryAddress)); // Address L2 memory to a pointer of the runtime
    std::memcpy(addressStart, sourceReg+TILE_DIM*i++, it->size);
  }
);

#endif