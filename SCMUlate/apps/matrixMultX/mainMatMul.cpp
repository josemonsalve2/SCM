#include <stdlib.h>
#include <stdio.h>
#include "MatMul.hpp"
#include "MatMulOMPoffload.hpp"
#include "scm_machine.hpp"
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <omp.h>
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
  bool fileInput = false;
  char * fileName;
  uint32_t MDIM_OPT;
  uint32_t NDIM_OPT;
  uint32_t KDIM_OPT;
} program_options;

 // 4 GB
#define SIZEOFMEM ((unsigned long)4e9)

void parseProgramOptions(int argc, char* argv[]);

void initMatrix (double * mat, int elements, int val = 0) {
  for (int i = 0; i < elements; i++)
    mat[i] = ((double)(i)*val)*0.00003;
}

void alg_matmul2D_serial(int m, int n, int p, double* a, double* b, double* c) {
   int i,j,k;
   for (i=0; i<m; i=i+1) {
      for (j=0; j<n; j=j+1) {
         for (k=0; k<p; k=k+1) {
            c[i*n + j]=(c[i*n + j])+((a[i*p + k])*(b[k*n + j]));
         }
      }
   }
}

template<typename T>
T changeEndiandness (T val) {
  T res = 0;
  uint8_t ptr[sizeof(T)]; 
  for (int i = sizeof(T)-1; i >=0 ; i--) {
    ptr[i] = val & 255;
    val = val >> 8;
  }
  res = reinterpret_cast<T*>(ptr)[0];
  return res;
}
// FUNCTION DEFINITIONS
int SCMUlate();

int main (int argc, char * argv[]) {
  unsigned char * memory;
  // argv[1] = M_tiles, argv[2] = N_tiles, argv[3] = K_tiles
  try {
    memory = new unsigned char[SIZEOFMEM];
  }
  catch (int e) {
    std::cout << "An exception occurred. Exception Nr. " << e << '\n';
    return 1;
  }

  parseProgramOptions(argc, argv);
  MDIM = program_options.MDIM_OPT;
  NDIM = program_options.NDIM_OPT;
  KDIM = program_options.KDIM_OPT;

  double * warmA = new double[NumElements_A];
  double * warmB = new double[NumElements_B];
  double * warmC = new double[NumElements_C];
  
  scm::codelet_params vars;
  vars.getParamAs(1) = reinterpret_cast<unsigned char*>(warmA); // Getting register 1
  vars.getParamAs(2) = reinterpret_cast<unsigned char*>(warmB); // Getting register 2
  vars.getParamAs(3) = reinterpret_cast<unsigned char*>(warmC); // Getting register 3
#ifdef MKL
#ifdef DECLARE_VARIANT
  // OMP TARGET WARM UP
  int isDevice= -1;
#pragma omp target map(tofrom:isDevice)
  isDevice = omp_is_initial_device();
  printf("Is running in the %s\n", (isDevice? "Host": "Device"));
  scm::_cod_MatMultGPU_2048L warmCodGPU(vars);
  warmCodGPU.implementation();
#endif
#endif
  scm::_cod_MatMult_2048L warmCod(vars);
  warmCod.implementation();

  
  // M, N, K are expressed in number of tiles
  // Address A, B, C are initial locations of the matrices in L1
  // Offset cbi, aj, bk, ak, cj are the number of bytes between tiles in each direction 
  // Number of elements between the tiles for LoadSqrTile 

  uint64_t *M = reinterpret_cast<uint64_t*> (memory); 
  uint64_t *N = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)]); 
  uint64_t *K = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*2]); 

  uint64_t *Add_a = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*3]);
  uint64_t *Add_b = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*4]);
  uint64_t *Add_c = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*5]); 

  uint64_t *Off_cbi = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*6]); 
  uint64_t *Off_aj = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*7]);
  uint64_t *Off_bk = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*8]); 
  uint64_t *Off_ak = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*9]); 
  uint64_t *Off_cj = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*10]);

  uint64_t *Off_a = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*11]);
  uint64_t *Off_b = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*12]);
  uint64_t *Off_c = reinterpret_cast<uint64_t*> (&memory[sizeof(uint64_t)*13]); 
  
  *M = MDIM;
  *N = NDIM;
  *K = KDIM;
  *Add_a = A_offset;
  *Add_b = B_offset;
  *Add_c = C_offset;
  *Off_cbi = TILE_DIM*sizeof(double);
  *Off_aj = KDIM*REG_SIZE;
  *Off_bk = NDIM*REG_SIZE;
  *Off_ak = TILE_DIM*sizeof(double);
  *Off_cj = NDIM*REG_SIZE;
  *Off_a = KDIM*TILE_DIM;
  *Off_b = NDIM*TILE_DIM;
  *Off_c = NDIM*TILE_DIM;
  
  #ifndef NOBLAS
    std::cout << "Using BLAS"<< std::endl;
  #else
    std::cout << "Not using BLAS"<< std::endl;
  #endif
  
  
  std::cout << "MDIM = " << MDIM << std::endl
            << "NDIM = " << NDIM << std::endl
            << "KDIM = " << KDIM << std::endl
            << "A_OFFSET = " << A_offset << std::endl
            << "B_OFFSET = " << B_offset << std::endl
            << "C_OFFSET = " << C_offset << std::endl
            << "*M = " << *M << std::endl
            << "*N = " << *N << std::endl
            << "*K = " << *K << std::endl
            << "*Add_a = " << *Add_a << std::endl
            << "*Add_b = " << *Add_b << std::endl
            << "*Add_c = " << *Add_c << std::endl
            << "*Off_cbi = " << *Off_cbi << std::endl
            << "*Off_aj = " << *Off_aj << std::endl
            << "*Off_bk = " << *Off_bk << std::endl
            << "*Off_ak = " << *Off_ak << std::endl
            << "*Off_cj = " << *Off_cj << std::endl
            << "*Off_a = " << *Off_a << std::endl
            << "*Off_b = " << *Off_b << std::endl
            << "*Off_c = " << *Off_c << std::endl
            << "Num_elements_A = " << NumElements_A << std::endl
            << "Num_elements_B = " << NumElements_B << std::endl
            << "Num_elements_C = " << NumElements_C << std::endl;



  double *A = reinterpret_cast<double*> (&memory[A_offset]); 
  double *B = reinterpret_cast<double*> (&memory[B_offset]); 
  double *C = reinterpret_cast<double*> (&memory[C_offset]);
  double *testC = new double[NumElements_C];

  initMatrix(A,NumElements_A,1);
  initMatrix(B,NumElements_B,1);
  initMatrix(C,NumElements_C,1);
  initMatrix(testC,NumElements_C,1);


  *M = changeEndiandness(*M) ;
  *N = changeEndiandness(*N) ;
  *K = changeEndiandness(*K) ;
  *Add_a = changeEndiandness(*Add_a) ;
  *Add_b = changeEndiandness(*Add_b) ;
  *Add_c = changeEndiandness(*Add_c) ;
  *Off_cbi = changeEndiandness(*Off_cbi) ;
  *Off_aj = changeEndiandness(*Off_aj) ;
  *Off_bk = changeEndiandness(*Off_bk) ;
  *Off_ak = changeEndiandness(*Off_ak) ;
  *Off_cj = changeEndiandness(*Off_cj) ;
  *Off_a = changeEndiandness(*Off_a) ;
  *Off_b = changeEndiandness(*Off_b) ;
  *Off_c = changeEndiandness(*Off_c) ;

  // SCM MACHINE
  scm::scm_machine * myMachine;
  if (program_options.fileInput) {
    SCMULATE_INFOMSG(0, "Reading program file %s", program_options.fileName);
    myMachine = new scm::scm_machine(program_options.fileName, memory, scm::OOO);
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

  std::chrono::time_point<std::chrono::high_resolution_clock> initTimer =  std::chrono::high_resolution_clock::now();
  #ifndef NOBLAS
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, MDIM*TILE_DIM, NDIM*TILE_DIM, KDIM*TILE_DIM, 1, A, TILE_DIM*KDIM, B, TILE_DIM*NDIM, 1, testC, TILE_DIM*NDIM);
  #else
    alg_matmul2D_serial(MDIM*TILE_DIM, NDIM*TILE_DIM, KDIM*TILE_DIM, A, B, testC);
  #endif// aCod.implementation();
  std::chrono::time_point<std::chrono::high_resolution_clock> endTimer = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff =endTimer - initTimer;
  printf ("taking %f s\n", diff.count());

  int errors = 0;
  for (long unsigned i = 0; i < NumElements_C; ++i) {
    if (abs(C[i] - testC[i]) > 0.00001) {
      success = false;
      SCMULATE_ERROR(0, "RESULT ERROR in i = %ld, value C[i] = %f  vs testC[i] = %f", i, C[i], testC[i]);
      if (++errors > 1) break;
    }
  }
  if (success)
    printf("SUCCESS!!!\n");
  delete myMachine;
  delete [] memory;
  delete [] testC;
  delete [] warmA;
  delete [] warmB;
  delete [] warmC;
  return 0;
}

void parseProgramOptions(int argc, char* argv[]) {
  // there are other arguments
  program_options.MDIM_OPT = 1;
  program_options.NDIM_OPT = 1;
  program_options.KDIM_OPT = 1;

  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      program_options.fileInput = true;
      program_options.fileName = argv[++i];
    }
    if (strcmp(argv[i], "-M") == 0) {
      program_options.MDIM_OPT = std::atoi(argv[++i]);
    }
    if (strcmp(argv[i], "-N") == 0) {
      program_options.NDIM_OPT = std::atoi(argv[++i]);
    }
    if (strcmp(argv[i], "-K") == 0) {
      program_options.KDIM_OPT = std::atoi(argv[++i]);
    }
  }
}

