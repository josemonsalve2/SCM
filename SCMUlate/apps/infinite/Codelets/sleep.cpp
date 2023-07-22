#include "sleep.hpp"
#include <chrono>
#include <thread>

#define SLEEP_TIME_MS 5

IMPLEMENT_CODELET(
    Sleep_2048L, double *A = this->getParams().getParamValueAs<double *>(2);
    double *B = this->getParams().getParamValueAs<double *>(3);
    double *C = this->getParams().getParamValueAs<double *>(1);

    // Sleep for x ms
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MS));

);
