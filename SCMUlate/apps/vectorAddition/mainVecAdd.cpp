#include <stdlib.h>
#include <stdio.h>
#include "vecAdd.hpp"
#include "scm_machine.hpp"
#include <cstring>
#include <iostream>

// Reps
#define REPS 400
//B offset = sizeRegister*400 = 52428800
#define B_offset (64*2048*REPS)
//C offset = sizeRegister*400*2 = 104857600
#define C_offset (64*2048*REPS*2)
#define NumElements ((64*2048*REPS)/sizeof(double))

static struct {
  bool fileInput = false;
  char * fileName;
} program_options;

 // 4 GB
#define SIZEOFMEM (unsigned long)4e9 

void parseProgramOptions(int argc, char* argv[]);

// FUNCTION DEFINITIONS
int SCMUlate();

int main (int argc, char * argv[]) {
  unsigned char * memory;
  parseProgramOptions(argc, argv);
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
  // SCM MACHINE
  scm::scm_machine * myMachine;
  if (program_options.fileInput) {
    SCMULATE_INFOMSG(0, "Reading program file %s", program_options.fileName);
    myMachine = new scm::scm_machine(program_options.fileName, memory);
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
  for (long unsigned i = 0; i < NumElements; ++i) {
    if (C[i] != i+i) {
      success = false;
      SCMULATE_ERROR(0, "RESULT ERROR in i = %ld, value C[i] = %f", i, C[i]);
      break;  
    }
  }
  if (success)
    printf("SUCCESS!!!\n");
  delete myMachine;
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
