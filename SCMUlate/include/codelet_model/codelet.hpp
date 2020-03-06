#ifndef __CODELET__
#define __CODELET__

#include <map>

namespace scm {

  class codelet;

  typedef codelet* (*codeletFactory)();

  class codelet {
    static std::map<std::string, codeletFactory> codeletsMap;
    private:
      uint32_t numParams;
      unsigned char * params;

      virtual void implementation(unsigned char * params) = 0;
    public:
      codelet() = delete;
      codelet(uint32_t numParams);

      void setParams(unsigned char * params);

      void fire() { this->implementation(this->params); };
  };
  
}

#endif
