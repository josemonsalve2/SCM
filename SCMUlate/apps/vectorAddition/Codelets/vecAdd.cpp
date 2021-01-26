#include "vecAdd.hpp"
#include <iomanip>

#define numElements 2048*64/sizeof(double)

IMPLEMENT_CODELET(vecAdd_2048L,
  double *A = this->getParams().getParamValueAs<double *>(2); // Getting register 1
  double *B = this->getParams().getParamValueAs<double *>(3); // Getting register 2
  double *C = this->getParams().getParamValueAs<double *>(1); // Getting register 3

  for (uint64_t i = 0; i < numElements; i++)
    C[i] = A[i] + B[i];
);