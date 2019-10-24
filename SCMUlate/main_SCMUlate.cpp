#include <stdlib.h>
#include <stdio.h>
#include "scm_machine.hpp"
#include <cstring>
#include <iostream>

static struct {
  bool fileInput = false;
  char * fileName;
} program_options;

void parseProgramOptions(int argc, char* argv[]);

// FUNCTION DEFINITIONS
int SCMUlate();

int main (int argc, char * argv[]) {
  parseProgramOptions(argc, argv);

  // SCM MACHINE
  scm::scm_machine * myMachine;
  if (program_options.fileInput) {
    SCMULATE_INFOMSG(0, "Reading program file %s", program_options.fileName);
    myMachine = new scm::scm_machine(program_options.fileName);
  } else {
    SCMULATE_INFOMSG(0, "Reading from stdin");
    myMachine = new scm::scm_machine("");
  }

  myMachine->run();

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
