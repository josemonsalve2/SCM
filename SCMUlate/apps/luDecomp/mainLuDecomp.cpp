#include <stdlib.h>
#include <stdio.h>
#include "LuDecomp.hpp"
#include "scm_machine.hpp"
#include <cstring>
#include <iostream>
#include <math.h>
//#include <libgen.h>

float ** BENCH;

// don't touch
static uint64_t current_malloc_size;
static unsigned char * malloc_root;
static uint64_t max_malloc_size;

static struct {
  bool fileInput = false;
  char * fileName;
} program_options;

 // 4 GB
#define SIZEOFMEM ((unsigned long)4e9)

void parseProgramOptions(int argc, char* argv[]);

// FUNCTION DEFINITIONS
int SCMUlate();
// api for local malloc
void lu_malloc_init(unsigned char * memory, uint64_t size);
void * lu_malloc(uint64_t size);
// functions defs from bots sparse LU decomposition benchmark
void genmat (float *M[]);
float * allocate_clean_block();
void pre_allocate(float **BENCH);
void print_structure(char *name, float *M[]);
void sparselu_init (float ***pBENCH, char *pass);

int main (int argc, char * argv[]) {
  unsigned char * memory;
  
  parseProgramOptions(argc, argv);
  // For now we only support the one file
  if (strcmp(program_options.fileName, "luDecomp.scm") != 0){
    std::cout << "Unsupported SCM file" << std::endl;
    return 1;
  }
  try {
    memory = new unsigned char[SIZEOFMEM];
    lu_malloc_init(memory, SIZEOFMEM);
    // do some setup here
    sparselu_init(&BENCH, "benchmark");
    genmat(BENCH);
    pre_allocate(BENCH);
  }
  catch (int e) {
    std::cout << "An exception occurred. Exception Nr. " << e << '\n';
    return 1;
  }
  //scm::codelet_params vars;
  //vars.getParamAs(1) = reinterpret_cast<unsigned char*>(warmA); // Getting register 1
  //vars.getParamAs(2) = reinterpret_cast<unsigned char*>(warmB); // Getting register 2
  //vars.getParamAs(3) = reinterpret_cast<unsigned char*>(warmC); // Getting register 3

  // SCM MACHINE
  scm::scm_machine * myMachine;
  if (program_options.fileInput) {
    SCMULATE_INFOMSG(0, "Reading program file %s", program_options.fileName);
    //myMachine = new scm::scm_machine(program_options.fileName, memory, scm::OOO);
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

  std::chrono::time_point<std::chrono::high_resolution_clock> initTimer =  std::chrono::high_resolution_clock::now();
  // aCod.implementation();
  std::chrono::time_point<std::chrono::high_resolution_clock> endTimer = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff =endTimer - initTimer;
  printf ("taking %f s\n", diff.count());

  int errors = 0;
  /*
  for (long unsigned i = 0; i < NumElements_C; ++i) {
    if (abs(C[i] - testC[i]) > 0.00001) {
      success = false;
      SCMULATE_ERROR(0, "RESULT ERROR in i = %ld, value C[i] = %f  vs testC[i] = %f", i, C[i], testC[i]);
      if (++errors > 256) break;
    }
  }
  if (success)
    printf("SUCCESS!!!\n");
  */
  delete myMachine;
  delete [] memory;
  //delete [] testC;
  return 0;
}

void lu_malloc_init(unsigned char * memory, uint64_t size) {
  malloc_root = memory;
  max_malloc_size = size;
  current_malloc_size = 0;
}

void * lu_malloc(uint64_t size) {
  // if overflow
  if (current_malloc_size + size >= max_malloc_size) {
    printf("Asking to allocate more memory than is available!\n");
    return(nullptr);
  }
  else {
    void * toReturn = (void *) &(malloc_root[current_malloc_size]);
    current_malloc_size = current_malloc_size + size;
    return(toReturn);
  }
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

void genmat (float *M[])
{
   int null_entry, init_val, i, j, ii, jj;
   float *p;

   init_val = 1325;

   /* generating the structure */
   for (ii=0; ii < bots_arg_size; ii++) // crosses full matrix size: ii/jj indicate address of submatrices?
   {
      for (jj=0; jj < bots_arg_size; jj++)
      {
         /* computing null entries */
         null_entry=FALSE;
         if ((ii<jj) && (ii%3 !=0)) null_entry = TRUE; // deciding with ifs whether submatrix has data or not
         if ((ii>jj) && (jj%3 !=0)) null_entry = TRUE; 
	 if (ii%2==1) null_entry = TRUE; 
	 if (jj%2==1) null_entry = TRUE; 
	 if (ii==jj) null_entry = FALSE; 
	 if (ii==jj-1) null_entry = FALSE; 
         if (ii-1 == jj) null_entry = FALSE;  
         /* allocating matrix */
         if (null_entry == FALSE){
            M[ii*bots_arg_size+jj] = (float *) lu_malloc(bots_arg_size_1*bots_arg_size_1*sizeof(float));
	    if (M[ii*bots_arg_size+jj] == NULL) // error checking for malloc / mem space
            {
               printf("Error: Out of memory\n");
               exit(101);
            }
            /* initializing matrix */
            p = M[ii*bots_arg_size+jj];
            for (i = 0; i < bots_arg_size_1; i++) // setting values for valid submatrices
            {
               for (j = 0; j < bots_arg_size_1; j++)
               {
	            init_val = (3125 * init_val) % 65536;
      	            (*p) = (float)((init_val - 32768.0) / 16384.0);
                    p++;
               }
            }
         }
         else
         {
            M[ii*bots_arg_size+jj] = NULL;
         }
      }
   }
}

float * allocate_clean_block()
{
  int i,j;
  float *p, *q;

  p = (float *) lu_malloc(bots_arg_size_1*bots_arg_size_1*sizeof(float));
  q=p;
  if (p!=NULL){
     for (i = 0; i < bots_arg_size_1; i++) 
        for (j = 0; j < bots_arg_size_1; j++){(*p)=0.0; p++;}
	
  }
  else
  {
      printf("Error: Out of memory\n");
      exit (101);
  }
  return (q);
}

// added to avoid having to allocate submatrices during the LU routine
// currently, allocating heap data in a memory codelet may cause some memory consistency issues
// is this really the case though if all it does is malloc? Malloc is already thread safe as far
// as I know, and the memory codelet itself would not actually read or write the space that was 
// allocated, only store the address in a register so that the memory codelet after the actual
// computation can write back to memory where necessary. Further discussion needed
void pre_allocate(float **BENCH)
{
  int ii, jj, kk;
  for (kk=0; kk<bots_arg_size; kk++)
    for (ii=1; ii<bots_arg_size; ii++)
      if (BENCH[ii*bots_arg_size+kk] != NULL) // these if statements will each be a memory codelet
        for (jj=1; jj<bots_arg_size; jj++)
          if (BENCH[kk*bots_arg_size+jj] != NULL) // this mem codelet should also check the below condition and perform allocation
          {
            // we don't want memory codelets to be performing memory allocation, only memory movement
            // so, in the main program outside of the SCM machine, we iterate through and allocate these 
            // blocks beforehand, so we don't have to do it during
            if (BENCH[ii*bots_arg_size+jj]==NULL) BENCH[ii*bots_arg_size+jj] = allocate_clean_block(); //allocate_clean_block -- mem codelet
          }
}

void print_structure(char *name, float *M[])
{
   int ii, jj;
   printf("Structure for matrix %s @ 0x%p\n",name, M);
   for (ii = 0; ii < bots_arg_size; ii++) {
     for (jj = 0; jj < bots_arg_size; jj++) {
        if (M[ii*bots_arg_size+jj]!=NULL) {printf("x");}
        else printf(" ");
     }
     printf("\n");
   }
   printf("\n");
}

void sparselu_init (float ***pBENCH, char *pass)
{
   *pBENCH = (float **) lu_malloc(bots_arg_size*bots_arg_size*sizeof(float *));
   genmat(*pBENCH);
   print_structure(pass, *pBENCH);
}
