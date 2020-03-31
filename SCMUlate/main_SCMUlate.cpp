#include <stdlib.h>
#include <stdio.h>
#include "scm_machine.hpp"
#include <cstring>
#include <iostream>

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
  parseProgramOptions(argc, argv);
  unsigned char * memory = new unsigned char[SIZEOFMEM];

  // SCM MACHINE
  scm::scm_machine * myMachine;
  if (program_options.fileInput) {
    SCMULATE_INFOMSG(0, "Reading program file %s", program_options.fileName);
    myMachine = new scm::scm_machine(program_options.fileName, memory);
  } else {
    SCMULATE_INFOMSG(0, "Reading from stdin");
    char emptyStr[10] = "";
    myMachine = new scm::scm_machine(emptyStr, memory);
  }

  myMachine->run();
  TIMERS_COUNTERS_GUARD(
    myMachine->setTimersOutput("trace.json");
  );
  delete myMachine;
  delete[] memory;
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
