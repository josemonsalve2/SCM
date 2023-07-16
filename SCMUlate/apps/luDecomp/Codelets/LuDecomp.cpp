#include "LuDecomp.hpp"
#include <iomanip>

// BENCH is externed so now we can access it directly instead of using a base register

// sizes of codelets? big questions here
// col -- read/write
// diag -- read
IMPLEMENT_CODELET(fwd_2048L,
  float *col = this->getParams().getParamValueAs<float*>(1);
  float *diag = this->getParams().getParamValueAs<float*>(2);

  int i;
  int j;
  int k;
  for (j=0; j<bots_arg_size_1; j++)
    for (k=0; k<bots_arg_size_1; k++) 
        for (i=k+1; i<bots_arg_size_1; i++)
          col[i*bots_arg_size_1+j] = col[i*bots_arg_size_1+j] - diag[i*bots_arg_size_1+k]*col[k*bots_arg_size_1+j];
);

IMPLEMENT_CODELET(bdiv_2048L,
  float *row = this->getParams().getParamValueAs<float*>(1);
  float *diag = this->getParams().getParamValueAs<float*>(2);

  int i; 
  int j;
  int k;
  for (i=0; i<bots_arg_size_1; i++)
    for (k=0; k<bots_arg_size_1; k++)
    {
      row[i*bots_arg_size_1+k] = row[i*bots_arg_size_1+k] / diag[k*bots_arg_size_1+k];
      for (j=k+1; j<bots_arg_size_1; j++)
        row[i*bots_arg_size_1+j] = row[i*bots_arg_size_1+j] - row[i*bots_arg_size_1+k]*diag[k*bots_arg_size_1+j];
    }
);

IMPLEMENT_CODELET(bmod_2048L,
  float *inner = this->getParams().getParamValueAs<float*>(1);
  float *row = this->getParams().getParamValueAs<float*>(2);
  float *col = this->getParams().getParamValueAs<float*>(3);

  int i; 
  int j;
  int k;
  for (i=0; i<bots_arg_size_1; i++)
    for (j=0; j<bots_arg_size_1; j++)
      for (k=0; k<bots_arg_size_1; k++)
        inner[i*bots_arg_size_1+j] = inner[i*bots_arg_size_1+j] - row[i*bots_arg_size_1+k]*col[k*bots_arg_size_1+j];
);

IMPLEMENT_CODELET(lu0_2048L,
  float *diag = this->getParams().getParamValueAs<float*>(1);

  int i; 
  int j;
  int k;
  for (k=0; k<bots_arg_size_1; k++)
    for (i=k+1; i<bots_arg_size_1; i++)
    {
      diag[i*bots_arg_size_1+k] = diag[i*bots_arg_size_1+k] / diag[k*bots_arg_size_1+k];
      for (j=k+1; j<bots_arg_size_1; j++)
      diag[i*bots_arg_size_1+j] = diag[i*bots_arg_size_1+j] - diag[i*bots_arg_size_1+k] * diag[k*bots_arg_size_1+j];
    }
);

// takes an offset kk which is used to locate the submatrix
// should be used before the lu0 codelet to load the data it needs
MEMRANGE_CODELET(loadSubMat_2048L,
  //float ** address_reg = this->getParams().getParamValueAs<float **>(2);
  uint64_t row_offset = *(this->getParams().getParamValueAs<uint64_t*>(2)); // kk in the original code
  uint64_t col_offset = *(this->getParams().getParamValueAs<uint64_t*>(3));
  float ** bench_addr = BENCH;
  uint64_t actual_offset = (row_offset * bots_arg_size + col_offset); // original contains bots_arg_size which is 50; define at top
  // Add range that will be touched (range of sub matrix)
  // lu0 goes from the start of its row 
  //printf("BENCH is %p\n", BENCH);
  //printf("actual offset: 0x%lx\n", actual_offset);
  //printf("bench + offset is %p\n", (bench_addr+actual_offset));
  //printf("submatrix pointer is %p\n", BENCH[actual_offset]);
  uint64_t submat = (uint64_t) BENCH[actual_offset];
  // BENCH should be the first thing allocated in the SCM memory space, so the offset to the submatrix is submat pointer - BENCH
  uint64_t mem_offset = submat - (uint64_t)BENCH;
  this->addReadMemRange(mem_offset, bots_arg_size_1*bots_arg_size_1);
  //this->addReadMemRange((uint64_t)BENCH[actual_offset], bots_arg_size_1*(bots_arg_size_1-1));
);

IMPLEMENT_CODELET(loadSubMat_2048L,
  float * destReg = this->getParams().getParamValueAs<float *>(1); 
  float * addressStart = reinterpret_cast<float *>(getAddress(memoryRanges->reads.begin()->memoryAddress));
  //printf("destReg @ %p; submatrix in memory @ %p\n", destReg, addressStart);
  std::memcpy(destReg, addressStart, memoryRanges->reads.begin()->size);
);

MEMRANGE_CODELET(storeSubMat_2048L,
  uint64_t row_offset = *(this->getParams().getParamValueAs<uint64_t*>(2)); // kk in the original code
  uint64_t col_offset = *(this->getParams().getParamValueAs<uint64_t*>(3));
  float ** bench_addr = BENCH;
  uint64_t actual_offset = (row_offset * bots_arg_size + col_offset); // original contains bots_arg_size which is 50; define at top
  // Add range that will be touched (range of sub matrix)
  // lu0 goes from the start of its row 
  //printf("BENCH is %p\n", BENCH);
  //printf("actual offset: %lx\n", actual_offset);
  //printf("bench + offset is %p\n", (bench_addr+actual_offset));
  //printf("submatrix pointer is %p\n", BENCH[actual_offset]);
  uint64_t submat = (uint64_t) BENCH[actual_offset];
  // BENCH should be the first thing allocated in the SCM memory space, so the offset to the submatrix is submat pointer - BENCH
  uint64_t mem_offset = submat - (uint64_t)BENCH;
  this->addWriteMemRange(mem_offset, bots_arg_size_1*bots_arg_size_1);
);

IMPLEMENT_CODELET(storeSubMat_2048L,
/*
  float * destReg = this->getParams().getParamValueAs<float *>(1); 
  float * addressStart = reinterpret_cast<float *>(getAddress(memoryRanges->reads.begin()->memoryAddress));
  std::memcpy(addressStart, destReg, memoryRanges->reads.begin()->size);
*/
  float * sourceReg = this->getParams().getParamValueAs<float *>(1); 
  float * addressStart = reinterpret_cast<float *>(getAddress(memoryRanges->writes.begin()->memoryAddress));
  //printf("sourceReg @ %p; submatrix in memory @ %p\n", sourceReg, addressStart);
  std::memcpy(sourceReg, addressStart, memoryRanges->writes.begin()->size);
);

// yes, this is a memory codelet that only exists to load the address of BENCH into a register
// yes, this is hacky
// yes, I'm sure there IS a better way to do this
// no, I'm not the one that will be doing it right now
MEMRANGE_CODELET(loadBenchAddr_64B,
  this->addReadMemRange((uint64_t)&BENCH, sizeof(float**));
);

IMPLEMENT_CODELET(loadBenchAddr_64B,
  float *** destReg = this->getParams().getParamValueAs<float ***>(1);
  //printf("bench: %p\n", BENCH);
  std::memcpy(destReg, &BENCH, sizeof(float **));
);

/*
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
*/
