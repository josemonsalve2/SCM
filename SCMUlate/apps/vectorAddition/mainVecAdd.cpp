#include <stdlib.h>
#include <stdio.h>
#include "vecAdd.hpp"
#include "scm_machine.hpp"
#include <cstring>
#include <iostream>

// Repetitions in L1
#define REPS 4000
// Size of L1 register used
#define REG2048L 64*2048
// Size of each vector sizeRegister*4000 = 524288000 bytes
#define SIZE_VECT ((REG2048L*REPS)/sizeof(double))


static struct {
  bool fileInput = false;
  char * fileName;
} program_options;

struct  __attribute__((packed)) l2_memory {
  double A[SIZE_VECT];
  double B[SIZE_VECT];
  double C[SIZE_VECT];
};

 // 4 GB
#define SIZEOFMEM (unsigned long)4e9 

void parseProgramOptions(int argc, char* argv[]);

// FUNCTION DEFINITIONS
int SCMUlate();

int main (int argc, char * argv[]) {
  l2_memory * memory = new l2_memory;
  parseProgramOptions(argc, argv);
  
  double *A = memory->A; 
  double *B = memory->B; 
  double *C = memory->C; 

  for (unsigned long i = 0; i < SIZE_VECT; ++i) {
      A[i] = i;
      B[i] = i;
  }
  // SCM MACHINE
  scm::scm_machine * myMachine;
  if (program_options.fileInput) {
    SCMULATE_INFOMSG(0, "Reading program file %s", program_options.fileName);
    myMachine = new scm::scm_machine(program_options.fileName, (unsigned char *)memory, scm::OOO);
  } else {
    std::cout << "Need to give a file to read. use -i <filename>" << std::endl;
    return 1;
  }


  myMachine->run();

  TIMERS_COUNTERS_GUARD(
    myMachine->setTimersOutput("trace.json");
  );

  // Checking result
  bool success = true;
  for (long unsigned i = 0; i < SIZE_VECT; ++i) {
    if (C[i] != i+i) {
      success = false;
      printf("RESULT ERROR in i = %ld, value C[i] = %f\n", i, C[i]);
      SCMULATE_ERROR(0, "RESULT ERROR in i = %ld, value C[i] = %f", i, C[i]);
      break;  
    }
  }
  if (success)
    printf("SUCCESS!!!\n");
  delete myMachine;
  delete memory;
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
