#include "MatMulOMPoffload.hpp"
#include <iomanip>

#ifdef MKL
#include "mkl.h"
#include "mkl_cblas.h"
#include "mkl_omp_offload.h"

//#pragma omp requires unified_shared_memory

#define TILE_DIM 128

MEMRANGE_CODELET(LoadSqTileGPU_2048L, 
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

IMPLEMENT_CODELET(LoadSqTileGPU_2048L,
  double *destReg = this->getParams().getParamValueAs<double*>(1);

  int i = 0;
  for (auto it = memoryRanges->reads.begin(); it != memoryRanges->reads.end(); it++) {
    double *addressStart = reinterpret_cast<double *> (getAddress(it->memoryAddress)); // Address L2 memory to a pointer of the runtime
    std::memcpy(destReg+TILE_DIM*i++, addressStart, it->size);
  }
);

IMPLEMENT_CODELET(MatMultGPU_2048L,
  // Obtaining the parameters
  double *A = this->getParams().getParamValueAs<double*>(2);
  double *B = this->getParams().getParamValueAs<double*>(3);
  double *C = this->getParams().getParamValueAs<double*>(1);

_Pragma("omp target data map(to:A[0:TILE_DIM*TILE_DIM],B[0:TILE_DIM*TILE_DIM]) map(tofrom:C[0:TILE_DIM*TILE_DIM])")
    {
        // run gemm on gpu, use standard oneMKL interface within a variant dispatch construct
_Pragma("omp target variant dispatch use_device_ptr(A, B, C)")
        {
          cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, TILE_DIM, TILE_DIM, TILE_DIM,
                       1, A, TILE_DIM, B, TILE_DIM, 1, C, TILE_DIM);
        }
    }

);


MEMRANGE_CODELET(StoreSqTileGPU_2048L, 
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

IMPLEMENT_CODELET(StoreSqTileGPU_2048L,
  // Obtaining the parameters
  double *sourceReg = this->getParams().getParamValueAs<double*>(1);

  int i = 0;
  for (auto it = memoryRanges->writes.begin(); it != memoryRanges->writes.end(); it++) {
    double *addressStart = reinterpret_cast<double *> (getAddress(it->memoryAddress)); // Address L2 memory to a pointer of the runtime
    std::memcpy(addressStart, sourceReg+TILE_DIM*i++, it->size);
  }
);

#endif