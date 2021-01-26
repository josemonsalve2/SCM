#include <iostream>
#include "codelet.hpp"

DEFINE_CODELET(hello_world,3,0);
IMPLEMENT_CODELET(hello_world,
    unsigned char * param0 = this->getParams().getParamValueAs<unsigned char*>(1);
    unsigned int param1 = this->getParams().getParamValueAs<uint64_t>(2);
    float param2 = this->getParams().getParamValueAs<float>(3);

    std::cout << " param 0 = " << param0 << std::endl
              << " param 1 = " << param1 << std::endl
              << " param 2 = " << param2 << std::endl;
);


int main() {
    scm::codelet_params myParams;
    unsigned char * myChar;
    myChar = new unsigned char[10];
    myChar[0] = 'h';
    myChar[1] = 'e';
    myChar[2] = 'l';
    myChar[3] = 'l';
    myChar[4] = 'o';
    myChar[6] = '\000';

    myParams.getParamAs<unsigned char *>(1) = myChar;
    myParams.getParamAs<uint64_t>(2) = 2;
    myParams.getParamAs<float>(3) = 9.99;
    
    scm::codelet * myCodelet = scm::codeletFactory::createCodelet("hello_world", myParams);
    myCodelet->implementation();
    
    delete [] myChar;
    delete myCodelet;

    return 0;
}