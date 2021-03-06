#include <iostream>
#include <chrono>
#include <string>
#include <omp.h>
#include <math.h>

#ifdef MKL
#include <mkl.h>
#endif
#ifdef BLAS
#include <cblas.h>
#endif

// Reps
uint32_t MDIM;
uint32_t NDIM;
uint32_t KDIM;
#define REG_SIZE (64*2048)
//A offset = sizeRegister*n_args (64 is the cacheline size) (6 args)
#define A_offset (sizeof(uint64_t)*15)
//B offset = sizeRegister*TILES (64 is the cacheline size)
#define B_offset (REG_SIZE*MDIM*KDIM+A_offset)
//C offset = sizeRegister*TILES*2
#define C_offset (REG_SIZE*NDIM*KDIM+B_offset)
#define NumElements_A ((REG_SIZE*MDIM*KDIM)/sizeof(double))
#define NumElements_B ((REG_SIZE*NDIM*KDIM)/sizeof(double))
#define NumElements_C ((REG_SIZE*NDIM*MDIM)/sizeof(double))
#define TILE_DIM sqrt(REG_SIZE/sizeof(double))

static struct {
  uint32_t MDIM_OPT;
  uint32_t NDIM_OPT;
  uint32_t KDIM_OPT;
  uint64_t numThreads;
  } program_options;


 // 4 GB
#define SIZEOFMEM ((unsigned long)4e9)

void parseProgramOptions(int argc, char* argv[]);

void initMatrix (double * mat, int elements, int val = 0) {
  for (int i = 0; i < elements; i++)
    mat[i] = ((double)(i*val))*0.0001;
} 



int main (int argc, char * argv[]) {
  parseProgramOptions(argc, argv);

  MDIM = program_options.MDIM_OPT;
  NDIM = program_options.NDIM_OPT;
  KDIM = program_options.KDIM_OPT;

  unsigned char * memory;
  try
  {
    memory = new unsigned char[SIZEOFMEM];
  }
  catch (int e)
  {
    std::cout << "An exception occurred. Exception Nr. " << e << '\n';
    return 1;
  }

  double *A = reinterpret_cast<double*> (&memory[A_offset]); 
  double *B = reinterpret_cast<double*> (&memory[B_offset]); 
  double *C = reinterpret_cast<double*> (&memory[C_offset]);

  initMatrix(A,NumElements_A,1);
  initMatrix(B,NumElements_B,1);
  initMatrix(C,NumElements_C,1);
  
  
  std::cout << "MDIM = " << MDIM << std::endl
            << "NDIM = " << NDIM << std::endl
            << "KDIM = " << KDIM << std::endl
            << "A_OFFSET = " << A_offset << std::endl
            << "B_OFFSET = " << B_offset << std::endl
            << "C_OFFSET = " << C_offset << std::endl;


  std::chrono::time_point<std::chrono::high_resolution_clock> initTimer =  std::chrono::high_resolution_clock::now();
#ifndef NOBLAS

#if MKL
  mkl_set_num_threads(program_options.numThreads);
#endif
  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, MDIM*TILE_DIM, NDIM*TILE_DIM, KDIM*TILE_DIM, 1, A, TILE_DIM*KDIM, B, TILE_DIM*NDIM, 1, C, TILE_DIM*NDIM);
#endif
  // aCod.implementation();
  std::chrono::time_point<std::chrono::high_resolution_clock> endTimer = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = endTimer - initTimer;

  std::cout << "Time = " << diff.count() << std::endl;
  
  delete [] memory;
  return 0;
}


void parseProgramOptions(int argc, char* argv[]) {
  // there are other arguments
  program_options.MDIM_OPT = 1;
  program_options.NDIM_OPT = 1;
  program_options.KDIM_OPT = 1;
  program_options.numThreads = 1;

  for (int i = 1; i + 1 < argc; i++) {
    if (std::string(argv[i]) == std::string("-M")) {
      program_options.MDIM_OPT = std::atoi(argv[++i]);
    }
    if (std::string(argv[i]) == std::string("-N")) {
      program_options.NDIM_OPT = std::atoi(argv[++i]);
    }
    if (std::string(argv[i]) == std::string("-K")) {
      program_options.KDIM_OPT = std::atoi(argv[++i]);
    }
    if (std::string(argv[i]) == std::string("-p")) {
      program_options.numThreads = std::atoi(argv[++i]);
    }
  }
}
