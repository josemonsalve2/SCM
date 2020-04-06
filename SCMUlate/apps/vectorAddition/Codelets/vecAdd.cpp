#include "vecAdd.hpp"
#include <iomanip>

#define numElements 2048*64/sizeof(double)

IMPLEMENT_CODELET(vecAdd_2048L,
  // Obtaining the parameters
  unsigned char ** args = static_cast<unsigned char **>(this->getParams());
  unsigned char *reg1 = args[0]; // Getting register 1
  unsigned char *reg2 = args[1]; // Getting register 2
  unsigned char *reg3 = args[2]; // Getting register 3
  double *A = reinterpret_cast<double*>(reg2);
  double *B = reinterpret_cast<double*>(reg3);
  double *C = reinterpret_cast<double*>(reg1);

  for (uint64_t i = 0; i < numElements; i++)
    C[i] = A[i] + B[i];
);