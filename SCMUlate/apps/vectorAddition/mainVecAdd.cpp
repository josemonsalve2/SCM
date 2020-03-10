#include <stdlib.h>
#include <stdio.h>
#include "scm_machine.hpp"
#include "vecAdd.hpp"
#include <cstring>
#include <iostream>

//B offset = sizeRegister*4 = 524288
#define B_offset (64*2048*4)
//C offset = sizeRegister*4*2 = 1048576
#define C_offset (64*2048*4*2)
#define NumElements (524288/sizeof(int))

static struct {
  bool fileInput = false;
  char * fileName;
} program_options;

 // 4 GB
#define SIZEOFMEM (int)4e10 

void parseProgramOptions(int argc, char* argv[]);

// FUNCTION DEFINITIONS
int SCMUlate();

int main (int argc, char * argv[]) {
  unsigned char * memory;
  parseProgramOptions(argc, argv);
  try
  {
    memory = new unsigned char[SIZEOFMEM];
    std::cout << "Size of " << SIZEOFMEM << std::endl;
  }
  catch (int e)
  {
    std::cout << "An exception occurred. Exception Nr. " << e << '\n';
  }
  
  std::cout << "size of " << sizeof(unsigned char) << std::endl;
  int *A = reinterpret_cast<int*> (memory); 
  int *B = reinterpret_cast<int*> (&memory[B_offset]); 
  int *C = reinterpret_cast<int*> (&memory[C_offset]); 

  for (auto i = 0; i < NumElements; ++i) {
      A[i] = 1;
      B[i] = 1;
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

  // Checking result
  for (auto i = 0; i < NumElements; ++i) {
    if (C[i] != 2) {
      SCMULATE_ERROR(0, "RESULT ERROR in i = %d, value C[i] = %d", i, C[i]);
      break;  
    }
  }
  return 0;
}

void parseProgramOptions(int argc, char* argv[]) {
  // there are other arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      program_options.fileInput = true;
      program_options.fileName = argv[++i];
    }
  }
}
