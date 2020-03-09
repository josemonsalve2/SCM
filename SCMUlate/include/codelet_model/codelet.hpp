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
      ~codelet() { }
    public:
      codelet () {};
      codelet (uint32_t nparms, void * params): numParams(nparms), params(params) {};
      virtual void implementation() = 0;
  };

  typedef codelet* (*creatorFnc)(void *);

  class codeletFactory {
    private:
      static std::map <std::string, creatorFnc> registeredCodelets;
    public:
      static void registerCreator(std::string name, creatorFnc fnc);
      static codelet* createCodelet(std::string name, void * usedParams);
  };
}

// HELPER MACROS TO CREATE NEW CODELETS 

#define DEFINE_CODELET(name, nparms) \
  namespace scm { \
   class _cod_##name : public codelet { \
    public: \
      static bool hasBeenRegistered; \
      /* Constructors */ \
      _cod_##name () { \
        params = nullptr; \
        std::cout << "fdsafdsafdas"; \
        if(!hasBeenRegistered) { \
          registerCodelet(); \
          hasBeenRegistered = true; \
          SCMULATE_INFOMSG(2, "Registering codelet %s", #name ); \
        } \
      } \
      _cod_##name (void* parms) : codelet(nparms, parms) {} \
      \
      /* Helper functions */ \
      static codelet* codeletCreator(void * usedParams) ; \
      void registerCodelet() { \
        creatorFnc thisFunc = codeletCreator; \
        codeletFactory::registerCreator( #name, thisFunc); \
      }\
      \
      /* Implementation function */ \
      virtual void implementation(); \
      \
      /* destructor */ \
      ~_cod_##name() { \
        std::cout << "fdsafdasfdas" << std::end; \
        /* TODO: This needs to be fixed */  \
        unsigned char * args = static_cast<unsigned char *>(this->params); \
        delete args; \
      } \
   }; \
   _cod_##name _cod_##name_register; \
  }

#define IMPLEMENT_CODELET(name,code) \
  bool scm::_cod_##name::hasBeenRegistered = false; \
  scm::codelet* scm::_cod_##name::codeletCreator(void * usedParams) { \
    scm::codelet* res = new _cod_##name(usedParams); \
    return res; \
  } \
  void scm::_cod_##name::implementation() { code; } 
#endif
