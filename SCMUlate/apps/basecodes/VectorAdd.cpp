#include <iostream>
#include <chrono>
#include <string>
#include <omp.h>

// Reps
#define REPS 4000
//B offset = sizeRegister*400 = 52428800
#define B_offset (64*2048*REPS)
//C offset = sizeRegister*400*2 = 104857600
#define C_offset (64*2048*REPS*2)
#define NumElements ((64*2048*REPS)/sizeof(double))

#define SIZEOFMEM (unsigned long)4e9

int numThreads;

void SequentialVecAdd(double * A, double * B, double *C) {
    for (uint64_t i = 0; i < NumElements; ++i) {
        C[i] = A[i] + B[i];
    }
}

void ParallelVecAdd(double * A, double * B, double *C) {
  #pragma omp parallel for num_threads(numThreads)
  for (uint64_t i = 0; i < NumElements; ++i) {
      C[i] = A[i] + B[i];
  }
}



int main (int argc, char * argv[]) {
  bool parallel = false;
  numThreads = omp_get_max_threads();
  for (int i = 1; i + 1 < argc; i++) {
    if (std::string(argv[i]) == std::string("-p")) {
      if (i + 1 < argc)
        numThreads = std::stoi(std::string(argv[++i]));
      parallel = true;
    }
  }
  

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
  
  double *A = reinterpret_cast<double*> (memory); 
  double *B = reinterpret_cast<double*> (&memory[B_offset]); 
  double *C = reinterpret_cast<double*> (&memory[C_offset]); 

  for (unsigned long i = 0; i < NumElements; ++i) {
      A[i] = i;
      B[i] = i;
  }

  std::chrono::time_point<std::chrono::high_resolution_clock> initTimer =  std::chrono::high_resolution_clock::now();
  if (parallel)
    ParallelVecAdd(A,B,C);
  else
    SequentialVecAdd(A,B,C);
  // aCod.implementation();
  std::chrono::time_point<std::chrono::high_resolution_clock> endTimer = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = endTimer - initTimer;

  std::cout << "NumElements = " << NumElements << std::endl << "Time = " << diff.count() << std::endl;
  
  delete [] memory;
  return 0;
}
