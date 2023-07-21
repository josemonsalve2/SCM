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

IMPLEMENT_CODELET(zero_2048L,
  float *toZero = this->getParams().getParamValueAs<float*>(1);
  for (int i=0; i<bots_arg_size_1; i++) {
    for (int j=0; j<bots_arg_size_1; j++) {
      toZero[i*bots_arg_size_1+j] = 0.0;
    }
  }
);

// takes an offset kk which is used to locate the submatrix
// should be used before the lu0 codelet to load the data it needs
MEMRANGE_CODELET(loadSubMat_2048L,
  uint64_t row_offset = *(this->getParams().getParamValueAs<uint64_t*>(2)); // kk in the original code
  uint64_t col_offset = *(this->getParams().getParamValueAs<uint64_t*>(3));
  uint64_t actual_offset = (row_offset * bots_arg_size + col_offset); // original contains bots_arg_size which is 50; define at top
  // Add range that will be touched (range of sub matrix)
  uint64_t submat = (uint64_t) BENCH[actual_offset];
  // BENCH should be the first thing allocated in the SCM memory space, so the offset to the submatrix is submat pointer - BENCH
  uint64_t mem_offset = submat - (uint64_t)BENCH;
  this->addReadMemRange(mem_offset, bots_arg_size_1*bots_arg_size_1*sizeof(float)); // accounts for range in submatrix
  this->addReadMemRange(actual_offset*sizeof(float*), sizeof(float*)); // accounts for pointer being read in overall matrix
);

IMPLEMENT_CODELET(loadSubMat_2048L,
  float * destReg = this->getParams().getParamValueAs<float *>(1); 
  float * addressStart = reinterpret_cast<float *>(getAddress(memoryRanges->reads.rbegin()->memoryAddress));
  // if this memory address is 0, that means the pointer we read from BENCH was NULL and this is a critical error
  uint64_t bench_idx = (uint64_t) memoryRanges->reads.begin()->memoryAddress; // get index of bench we're reading submat pointer from for error checking
  SCMULATE_ERROR_IF(0, *((float**)(((unsigned char *)BENCH)+bench_idx)) == nullptr, "Error: loading submatrix from invalid pointer");
  std::memcpy(destReg, addressStart, memoryRanges->reads.rbegin()->size);
);

MEMRANGE_CODELET(storeSubMat_2048L,
  uint64_t row_offset = *(this->getParams().getParamValueAs<uint64_t*>(2)); // kk in the original code
  uint64_t col_offset = *(this->getParams().getParamValueAs<uint64_t*>(3));
  float ** bench_addr = BENCH;
  uint64_t actual_offset = (row_offset * bots_arg_size + col_offset); // original contains bots_arg_size which is 50; define at top
  // Add range that will be touched (range of sub matrix)
  uint64_t submat = (uint64_t) BENCH[actual_offset];
  // BENCH should be the first thing allocated in the SCM memory space, so the offset to the submatrix is submat pointer - BENCH
  uint64_t mem_offset = submat - (uint64_t)BENCH;
  this->addWriteMemRange(mem_offset, bots_arg_size_1*bots_arg_size_1*sizeof(float)); // account for writing range in the submatrix
  this->addReadMemRange(actual_offset*sizeof(float*), sizeof(float*)); // account for reading the pointer in the overarching matrix
);

IMPLEMENT_CODELET(storeSubMat_2048L,
  float * sourceReg = this->getParams().getParamValueAs<float *>(1); 
  float * addressStart = reinterpret_cast<float *>(getAddress(memoryRanges->writes.begin()->memoryAddress));
  uint64_t bench_idx = (uint64_t) memoryRanges->reads.begin()->memoryAddress; // get index of bench we're reading submat pointer from for error checking
  SCMULATE_ERROR_IF(0, *((float**)(((unsigned char *)BENCH)+bench_idx)) == nullptr, "Error: storing submatrix to invalid pointer");
  std::memcpy(addressStart, sourceReg, memoryRanges->writes.begin()->size);
);

// yes, this is a memory codelet that only exists to load the address of BENCH into a register
// yes, this is hacky
// yes, I'm sure there IS a better way to do this
// no, I'm not the one that will be doing it right now
MEMRANGE_CODELET(loadBenchAddr_64B,
  // BENCH is located at the beginning of SCM memory, hence an offset of 0
  this->addReadMemRange((uint64_t)0, sizeof(float**));
);

IMPLEMENT_CODELET(loadBenchAddr_64B,
  float *** destReg = this->getParams().getParamValueAs<float ***>(1);
  std::memcpy(destReg, &BENCH, sizeof(float **));
);

