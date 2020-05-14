#ifndef __CODELET__
#define __CODELET__

#include <map>
#include <iostream>
#include "SCMUlate_tools.hpp"
#include "instructions_def.hpp"

namespace scm {
  class cu_executor_module;
  class codelet {
    protected:
      uint32_t numParams;
      // we use void * because eventually we want this to be the interface for a compiler
      // generated code (e.g. pthreads equivalent). Also because we would like to provide
      // support for different parameters packing according to immediate vs register values. 
      // For now, we start with a registers only implementation
      void * params;
      std::uint_fast16_t op_in_out;
      cu_executor_module * myExecutor;
    public:
      codelet () : params(nullptr) {};
      codelet (uint32_t nparms, void * params, std::uint_fast16_t opIO): numParams(nparms), params(params), op_in_out(opIO) {};
      virtual void implementation() = 0;
      inline void * getParams() { return this->params; };
      inline std::uint_fast16_t& getOpIO() { return op_in_out; };
      inline void setExecutor (cu_executor_module * exec) {this->myExecutor = exec;}
      inline cu_executor_module * getExecutor() {return this->myExecutor;}
      l2_memory_t getAddress(uint64_t addr);
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
#define COD_INSTANCE_NAME(name, post) _cod_ ## name ## post

#define DEFINE_CODELET(name, nparms, opIO) \
  namespace scm { \
   class COD_CLASS_NAME(name) : public codelet { \
    public: \
      static bool hasBeenRegistered; \
      /* Constructors */ \
      static void codeletRegistrer() __attribute__((constructor)); \
      COD_CLASS_NAME(name) (void* parms) : codelet(nparms, parms, opIO) {} \
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
    void COD_CLASS_NAME(name)::codeletRegistrer() { \
        if(!hasBeenRegistered) { \
          registerCodelet(); \
          hasBeenRegistered = true; \
        } \
      } \
    codelet* COD_CLASS_NAME(name)::codeletCreator(void * usedParams) { \
      codelet* res = new COD_CLASS_NAME(name)(usedParams); \
      return res; \
    } \
    void COD_CLASS_NAME(name)::implementation() { code; } \
  }
#endif
