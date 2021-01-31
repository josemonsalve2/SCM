#include "MatMulOMPoffload.hpp"
#include <iomanip>

#ifdef MKL
#include "mkl.h"
#include "mkl_cblas.h"
#include "mkl_omp_offload.h"
#endif

//#pragma omp requires unified_shared_memory

#define TILE_DIM 128

MEMRANGE_CODELET(LoadSqTileGPU_2048L, 
  // Obtaining the parameters
  uint8_t* address_reg = this->getParams().getParamValueAs<uint8_t*>(2);
  uint8_t* ldistance_reg = this->getParams().getParamValueAs<uint8_t*>(3);

  uint64_t address = address_reg[0];
    uint64_t ldistance = ldistance_reg[0];
  for (int i = 1; i < 8; i++) {
    address <<= 8;
    address += address_reg[i];
    ldistance <<= 8;
    ldistance += ldistance_reg[i];
  }
  // Add the ranges
  for (uint64_t i = 0; i < TILE_DIM; i++) {
    this->addReadMemRange(address+ldistance*sizeof(double)*i, TILE_DIM*sizeof(double));
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

#ifndef NOBLAS
#if MKL
_Pragma("omp target data map(to:A[0:TILE_DIM*TILE_DIM],B[0:TILE_DIM*TILE_DIM]) map(tofrom:C[0:TILE_DIM*TILE_DIM])")
    {

        // run gemm on gpu, use standard oneMKL interface within a variant dispatch construct
_Pragma("omp target variant dispatch use_device_ptr(A, B, C)")
        {
          cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, TILE_DIM, TILE_DIM, TILE_DIM, 1, A, TILE_DIM, B, TILE_DIM, 1, C, TILE_DIM);
        }
    }
#endif
#else
_Pragma("omp target data map(to:A[0:TILE_DIM*TILE_DIM],B[0:TILE_DIM*TILE_DIM]) map(tofrom:C[0:TILE_DIM*TILE_DIM])")
    {
_Pragma("omp target teams distribute parallel for collapse(2)")
      {
        for (uint32_t i = 0; i < TILE_DIM; ++i) {
          for (uint32_t j = 0; j < TILE_DIM; ++j) {
            double res = 0;
            _Pragma("omp simd reduction(+:res)")
            for (uint32_t k = 0; k < TILE_DIM; ++k) {
              res += A[i*TILE_DIM + k]*B[k*TILE_DIM + j];
            }
            C[i*TILE_DIM + j] += res;
          }
        }
      }
    }
#endif
);

MEMRANGE_CODELET(StoreSqTileGPU_2048L, 
  // Obtaining the parameters
  uint8_t* address_reg = this->getParams().getParamValueAs<uint8_t*>(2);
  uint8_t* ldistance_reg = this->getParams().getParamValueAs<uint8_t*>(3);

  uint64_t address = address_reg[0];
  uint64_t ldistance = ldistance_reg[0];
  for (int i = 1; i < 8; i++) {
    address <<= 8;
    address += address_reg[i];
    ldistance <<= 8;
    ldistance += ldistance_reg[i];
  }
  for (uint64_t i = 0; i < TILE_DIM; i++) {
    this->addWriteMemRange(address+ldistance*sizeof(double)*i, TILE_DIM*sizeof(double));
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
