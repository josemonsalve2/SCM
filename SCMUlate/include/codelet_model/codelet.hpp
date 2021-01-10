#ifndef __CODELET__
#define __CODELET__

#include <map>
#include <set>
#include <iostream>
#include "SCMUlate_tools.hpp"
#include "instructions_def.hpp"

#define MAX_NUM_PARAMS_CODELETS 3
#define MAX_NUM_OPERANDS 3
namespace scm {
  class cu_executor_module;

  /** Codelet Parameters are packed in an array of unsigned char *
   * 
   * The value in each possition of the array is either a valid pointer too 
   * an unsigned char * that corresponds to a register, or it is an invalid 
   * pointer that is reinterpreted as a value such as an integer or a double
   * The implementation of the codelet uses the getParamsAs to determine how to
   * read the 64 or 32 bits (i.e. sizeof(unsigned char *) )
   * 
   */
  class codelet_params {
    private: 
      unsigned char** params;
      bool *isAddress;
      size_t size_params;
    public:
      codelet_params() {
        size_params = MAX_NUM_PARAMS_CODELETS * sizeof(unsigned char*);
        params = new unsigned char* [size_params];
        isAddress = new bool [MAX_NUM_PARAMS_CODELETS];
        std::memset(isAddress, 0, sizeof(bool)*MAX_NUM_PARAMS_CODELETS);
      }
      // Copy constructor
      codelet_params(const codelet_params &other) : size_params(other.size_params) {
        if (other.params != nullptr) {
          params = new unsigned char* [size_params];
          std::memcpy(this->params, other.params, other.size_params);
        } else {
          this->params = nullptr;
        }
        isAddress = new bool [MAX_NUM_PARAMS_CODELETS];
        std::memcpy(this->isAddress, other.isAddress, sizeof(bool)*MAX_NUM_PARAMS_CODELETS);
      }

      void inline setParamAsAddress(uint32_t bitmapParams) {
        int curParam = 0;
        while (bitmapParams != 0) {
          if (bitmapParams & 1)
            setOneParamAsAddress(curParam);
          curParam++;
          bitmapParams >>= 1;
        }
      }

      void inline setOneParamAsAddress(int op_num) {
        isAddress[op_num] = true;
      }

      // Params start in 1 in the caller
      bool inline isParamAnAddress(int op_num) {
        return isAddress[op_num-1];
      }
      // Get reference parameter
      template <class T = unsigned char *> 
      T & getParamAs(int paramNum) {
        return reinterpret_cast<T&>(params[paramNum]);
      }
      // Get constant reference parameter
      template <class T = unsigned char *> 
      T const & getParamAs(int paramNum) const {
        return reinterpret_cast<T&>(params[paramNum]);
      }

      // Get constant reference parameter
      template <class T = unsigned char *> 
      T getParamValueAs(int paramNum) {
        static_assert(sizeof(T) <= sizeof(unsigned char *), "The size of the parameter is larger than the pointer size ");
        // Pointer to certain values is not permited. So we take the paramNum
        // position and we cast it to a pointer of the type. Then we get the first
        // element. Be careful if the size of T is larger than sizeof (unsigned char *)
        return reinterpret_cast<T*>(params+paramNum)[0];
      }

      void * getParams() { return this->params; }
      ~codelet_params() {
        if (this->params != nullptr)
          delete[] params;
        delete[] isAddress;
      }
  };
  
  class codelet {
    protected:
      uint32_t numParams;
      memranges_pair * memoryRanges;
      // we use codelet_params because eventually we want this to be the interface for a compiler
      // generated code (e.g. pthreads equivalent). Also because we would like to provide
      // support for different parameters packing according to immediate vs register values. 
      // For now, we start with a registers only implementation
      codelet_params params;
      std::uint_fast16_t op_in_out;
      cu_executor_module * myExecutor;
    public:
      codelet () {};
      codelet (uint32_t nparms, codelet_params params, std::uint_fast16_t opIO) : numParams(nparms), memoryRanges(nullptr), params(params), op_in_out(opIO) {};
      codelet (const codelet &other) : numParams(other.numParams), memoryRanges(nullptr), params(other.params), op_in_out(other.op_in_out), myExecutor(other.myExecutor) {}
      virtual void implementation() = 0;
      virtual bool isMemoryCodelet() { return false; }
      virtual void calculateMemRanges() { };
      void setMemoryRange( memranges_pair * memRange ) { this->memoryRanges = memRange; };
      memranges_pair * getMemoryRange() { return memoryRanges; };
      bool isOpAnAddress(int op_num) { return params.isParamAnAddress(op_num); };
      inline codelet_params& getParams() { return this->params; };
      inline std::uint_fast16_t& getOpIO() { return op_in_out; };
      inline void setExecutor (cu_executor_module * exec) {this->myExecutor = exec;}
      inline cu_executor_module * getExecutor() {return this->myExecutor;}
      l2_memory_t getAddress(uint64_t addr);
      l2_memory_t getAddress(l2_memory_t addr);
      virtual ~codelet() { }
  };

  typedef codelet* (*creatorFnc)(codelet_params);

  class codeletFactory {
    public:
      static std::map <std::string, creatorFnc> *registeredCodelets;
      static void registerCreator(std::string name, creatorFnc fnc);
      static codelet* createCodelet(std::string name, codelet_params usedParams);
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
      COD_CLASS_NAME(name) (codelet_params parms) : codelet(nparms, parms, opIO) {} \
      COD_CLASS_NAME(name) (const COD_CLASS_NAME(name) &other) : codelet(other) {} \
      \
      /* Helper functions */ \
      static codelet* codeletCreator(codelet_params usedParams) ; \
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

#define DEFINE_MEMORY_CODELET(name, nparms, opIO, opAddr) \
  namespace scm { \
   class COD_CLASS_NAME(name) : public codelet { \
    public: \
      static bool hasBeenRegistered; \
      /* Constructors */ \
      static void codeletRegistrer() __attribute__((constructor)); \
      COD_CLASS_NAME(name) (codelet_params parms) : codelet(nparms, parms, opIO) { \
        params.setParamAsAddress(opAddr); \
      } \
      COD_CLASS_NAME(name) (const COD_CLASS_NAME(name) &other) : codelet(other) {} \
      \
      /* Helper functions */ \
      static codelet* codeletCreator(codelet_params usedParams) ; \
      static void registerCodelet() { \
        creatorFnc thisFunc = codeletCreator; \
        codeletFactory::registerCreator( #name, thisFunc); \
      }\
      void inline addReadMemRange(uint64_t start, uint32_t size) { \
        memoryRanges->reads.emplace(reinterpret_cast<l2_memory_t>(start), size); \
      } \
      void inline addWriteMemRange(uint64_t start, uint32_t size) { \
        memoryRanges->writes.emplace(reinterpret_cast<l2_memory_t>(start), size); \
      } \
      \
      /* Implementation function */ \
      virtual void implementation(); \
      virtual bool isMemoryCodelet() { return true; } \
      virtual void calculateMemRanges(); \
      \
      /* destructor */ \
      ~COD_CLASS_NAME(name)() {} \
    }; \
    \
  }

// Here the programmer should specify if one of the parameters is used as an address.
#define MEMRANGE_CODELET(name, code) \
    void \
    scm::COD_CLASS_NAME(name)::calculateMemRanges() {  \
      code; \
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
    codelet* COD_CLASS_NAME(name)::codeletCreator(codelet_params usedParams) { \
      codelet* res = new COD_CLASS_NAME(name)(usedParams); \
      return res; \
    } \
    void COD_CLASS_NAME(name)::implementation() { code; } \
  }
#endif
