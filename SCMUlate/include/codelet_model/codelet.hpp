#ifndef __CODELET__
#define __CODELET__

#include <map>
#include <iostream>
#include "SCMUlate_tools.hpp"

namespace scm {

  class codelet {
    protected:
      uint32_t numParams;
      // we use void * because eventually we want this to be the interface for a compiler
      // generated code (e.g. pthreads equivalent). Also because we would like to provide
      // support for different parameters packing according to immediate vs register values. 
      // For now, we start with a registers only implementation
      void * params;
    public:
      codelet () {};
      codelet (uint32_t nparms, void * params): numParams(nparms), params(params) {};
      virtual void implementation() = 0;
      void * getParams() { return this->params; };
      virtual ~codelet() { }
  };

  typedef codelet* (*creatorFnc)(void *);

  class codeletFactory {
    public:
      static std::map <std::string, creatorFnc> *registeredCodelets;
      static void registerCreator(std::string name, creatorFnc fnc);
      static codelet* createCodelet(std::string name, void * usedParams);
  };
}

// HELPER MACROS TO CREATE NEW CODELETS 

#define COD_CLASS_NAME(name) _cod_ ## name
#define COD_INSTANCE_NAME(name,post) _cod_ ## name ## post

#define DEFINE_CODELET(name, nparms) \
  namespace scm { \
   class COD_CLASS_NAME(name) : public codelet { \
    public: \
      static bool hasBeenRegistered; \
      /* Constructors */ \
      static void codeletRegistrer() __attribute__((constructor)) { \
        if(!hasBeenRegistered) { \
          registerCodelet(); \
          hasBeenRegistered = true; \
        } \
      } \
      COD_CLASS_NAME(name) (void* parms) : codelet(nparms, parms) {} \
      \
      /* Helper functions */ \
      static codelet* codeletCreator(void * usedParams) ; \
      static void registerCodelet() { \
        creatorFnc thisFunc = codeletCreator; \
        codeletFactory::registerCreator( #name, thisFunc); \
      }\
      \
      /* Implementation function */ \
      virtual void implementation(); \
      \
      /* destructor */ \
      ~COD_CLASS_NAME(name)() {} \
    }; \
    \
  }

#define IMPLEMENT_CODELET(name,code) \
  namespace scm {\
    bool COD_CLASS_NAME(name)::hasBeenRegistered = false; \
    codelet* COD_CLASS_NAME(name)::codeletCreator(void * usedParams) { \
      codelet* res = new COD_CLASS_NAME(name)(usedParams); \
      return res; \
    } \
    void COD_CLASS_NAME(name)::implementation() { code; } \
    \
  }
#endif
