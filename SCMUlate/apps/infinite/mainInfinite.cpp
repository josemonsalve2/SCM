#include "scm_machine.hpp"
#include "sleep.hpp"
#include <cstring>
#include <iostream>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

// Reps
#define REG_SIZE (64 * 2048)

static struct {
  bool fileInput = false;
  char *fileName;
} program_options;

// 4 GB
#define SIZEOFMEM ((unsigned long)4e9)

void parseProgramOptions(int argc, char *argv[]);

// FUNCTION DEFINITIONS
int SCMUlate();

int main(int argc, char *argv[]) {
  unsigned char *memory;

  parseProgramOptions(argc, argv);
  try {
    memory = new unsigned char[SIZEOFMEM];
  } catch (int e) {
    std::cout << "An exception occurred. Exception Nr. " << e << '\n';
    return 1;
  }

  // SCM MACHINE
  scm::scm_machine *myMachine;
  if (program_options.fileInput) {
    SCMULATE_INFOMSG(0, "Reading program file %s", program_options.fileName);
    myMachine =
        new scm::scm_machine(program_options.fileName, memory, scm::OOO);
  } else {
    std::cout << "Need to give a file to read. use -i <filename>" << std::endl;
    return 1;
  }

  if (myMachine->run() != scm::SCM_RUN_SUCCESS) {
    SCMULATE_ERROR(0, "THERE WAS AN ERROR WHEN RUNNING THE SCM MACHINE");
    return 1;
  }

  TIMERS_COUNTERS_GUARD(myMachine->setTimersOutput("trace.json"););

  delete myMachine;
  delete[] memory;
  return 0;
}

void parseProgramOptions(int argc, char *argv[]) {
  // there are other arguments
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      program_options.fileInput = true;
      program_options.fileName = argv[++i];
    }
  }
}
