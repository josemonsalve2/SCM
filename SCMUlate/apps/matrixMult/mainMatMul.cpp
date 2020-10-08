#include <stdlib.h>
#include <stdio.h>
#include "MatMul.hpp"
#include "MatMulOMPoffload.hpp"
#include "scm_machine.hpp"
#include <cstring>
#include <iostream>
#include <math.h>
#ifdef MKL
#include <mkl.h>
#endif
#ifdef BLAS
#include <cblas.h>
#endif

// Reps
uint32_t TILES;
uint32_t MDIM;
uint32_t NDIM;
uint32_t KDIM;
#define REG_SIZE (64*2048)
//B offset = sizeRegister*TILES (64 is the cacheline size)
#define B_offset (REG_SIZE*TILES)
//C offset = sizeRegister*TILES*2
#define C_offset (REG_SIZE*TILES *2)
#define NumElements_AB ((REG_SIZE*TILES)/sizeof(double))
#define TILE_DIM sqrt(REG_SIZE/sizeof(double))

static struct {
  bool fileInput = false;
  char * fileName;
} program_options;

 // 4 GB
#define SIZEOFMEM ((unsigned long)4e9)

void parseProgramOptions(int argc, char* argv[]);

// FUNCTION DEFINITIONS
int SCMUlate();

int main (int argc, char * argv[]) {
  unsigned char * memory;
  parseProgramOptions(argc, argv);
  // TODO: Harcoding these for now, until we have a general 
  if (strcmp(program_options.fileName, "matMul1tile.scm") == 0 || strcmp(program_options.fileName, "matMul1tileGPU.scm") == 0) {
    TILES = 1;
    MDIM = 1;
    NDIM = 1;
    KDIM = 1;
  } else if(strcmp(program_options.fileName, "matMul128x1280.scm") == 0 || strcmp(program_options.fileName, "matMul128x1280gpu.scm") == 0) {
    // A [10 x 1], B [1 x 10], C [1 x 1]
    TILES = 10;
    MDIM = 1;
    NDIM = 1;
    KDIM = 10;
  } else {
    std::cout << "Unsupported SCM file" << std::endl;
    return 1;
  }
  try {
    memory = new unsigned char[SIZEOFMEM];
  }
  catch (int e) {
    std::cout << "An exception occurred. Exception Nr. " << e << '\n';
    return 1;
  }

  std::cout << "TILES = " << TILES << std::endl
            << "MDIM = " << MDIM << std::endl
            << "NDIM = " << NDIM << std::endl
            << "KDIM = " << KDIM << std::endl
            << "B_OFFSET = " << B_offset << std::endl
            << "C_OFFSET = " << C_offset << std::endl;

  double *A = reinterpret_cast<double*> (memory); 
  double *B = reinterpret_cast<double*> (&memory[B_offset]); 
  double *C = reinterpret_cast<double*> (&memory[C_offset]);
  double *testC = new double[NumElements];

  for (unsigned long i = 0; i < NumElements; ++i) {
      A[i] = 1;
      B[i] = i;
      C[i] = 0;
      testC[i] = 0;
  }
  // SCM MACHINE
  scm::scm_machine * myMachine;
  if (program_options.fileInput) {
    SCMULATE_INFOMSG(0, "Reading program file %s", program_options.fileName);
    myMachine = new scm::scm_machine(program_options.fileName, memory, scm::SEQUENTIAL);
  } else {
    std::cout << "Need to give a file to read. use -i <filename>" << std::endl;
    return 1;
  }

  if (myMachine->run() != scm::SCM_RUN_SUCCESS) {
    SCMULATE_ERROR(0, "THERE WAS AN ERROR WHEN RUNNING THE SCM MACHINE");
    return 1;
  }

  TIMERS_COUNTERS_GUARD(
    myMachine->setTimersOutput("trace.json");
  );

  // Checking result
  bool success = true;
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, MDIM*TILE_DIM, NDIM*TILE_DIM, KDIM*TILE_DIM, 1, A, TILE_DIM*KDIM, B, TILE_DIM*NDIM, 0, testC, TILE_DIM*NDIM);
  int errors = 0;
  for (long unsigned i = 0; i < NumElements; ++i) {
    if (C[i] != testC[i]) {
      success = false;
      SCMULATE_ERROR(0, "RESULT ERROR in i = %ld, value C[i] = %f  vs testC[i] = %f", i, C[i], testC[i]);
      if (++errors > 10) break;  
    }
  }
  if (success)
    printf("SUCCESS!!!\n");
  delete myMachine;
  delete [] memory;
  return 0;
}

void parseProgramOptions(int argc, char* argv[]) {
  // there are other arguments
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      program_options.fileInput = true;
      program_options.fileName = argv[++i];
    }
  }
}

