//===------ SCMUlate_tools.h -----------------------------------===//
// 
// Different helper functions and macros for error handling and 
// other commonly used operations
// 
//===-----------------------------------------------------------===//
#ifndef __SCMULATE_TOOLS__
#define __SCMULATE_TOOLS__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "system_config.hpp"
#include "assert.h"

#ifdef DECLARE_VARIANT
#pragma omp requires unified_shared_memory
#endif
typedef unsigned char * l2_memory_t;

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#ifndef VERBOSE_MODE
#define VERBOSE_MODE -1
#endif

#ifdef PROFILER_INSTRUMENT
  #if __INTEL_LLVM_COMPILER
    #include <ittnotify.h>
    #define ITT_HANDLE_NAME(name) __scmulate_itt_handle_ ## name
    #define ITT_DOMAIN_VAR_NAME(name) __scmulate_itt_domain_ ## name
    #define ITT_LINE_NUMBER "_l" ## __LINE__
    #define ITT_DOMAIN_STR_NAME(name) "SCMULATE_DOM_" # name
    #define ITT_PAUSE __itt_pause();
    #define ITT_RESUME __itt_resume();
    #define ITT_DETACH __itt_detach();
    #define ITT_DOMAIN(name) __itt_domain* scmulate_itt_domain = __itt_domain_create(ITT_DOMAIN_STR_NAME(name));
    #define ITT_STR_HANDLE(name) __itt_string_handle* ITT_HANDLE_NAME(name) = __itt_string_handle_create(#name);
    #define ITT_TASK_BEGIN(domain, taskname) __itt_task_begin(scmulate_itt_domain, __itt_null, __itt_null, ITT_HANDLE_NAME(taskname));
    #define ITT_TASK_END(domain) __itt_task_end(scmulate_itt_domain);
  #else 
    #define ITT_PAUSE 
    #define ITT_RESUME 
    #define ITT_DETACH 
    #define ITT_DOMAIN(name) 
    #define ITT_STR_HANDLE(name) 
    #define ITT_TASK_BEGIN(domain, name) 
    #define ITT_TASK_END(name) 
  #endif
#else
  #define ITT_PAUSE 
  #define ITT_RESUME 
  #define ITT_DETACH 
  #define ITT_DOMAIN(name) 
  #define ITT_STR_HANDLE(name) 
  #define ITT_TASK_BEGIN(domain, name) 
  #define ITT_TASK_END(name) 
#endif

// Macro for output of information, warning and error messages
#if VERBOSE_MODE >= 0
  #define SCMULATE_WARNING(level, message, ...) { \
    if(VERBOSE_MODE >= level) {\
      printf("[SCMULATE_WARNING: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
    } \
  }
  #define SCMULATE_WARNING_IF(level, condition, message, ...) { \
    if(VERBOSE_MODE >= level && condition) { \
      printf("[SCMULATE_WARNING: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
    } \
  }
  
  #define SCMULATE_ERROR(level, message, ...) { \
    if(VERBOSE_MODE >= level) {\
      fprintf(stderr, "[SCMULATE_ERROR: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
      assert(0 && message); \
    } \
  }
  #define SCMULATE_ERROR_IF(level, condition, message, ...) { \
    if(VERBOSE_MODE >= level && condition) { \
      fprintf(stderr, "[SCMULATE_ERROR: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
      assert(0 && message); \
    } \
  }
  
  #define SCMULATE_INFOMSG(level, message, ...) { \
    if(VERBOSE_MODE >= level) {\
      printf("[SCMULATE_INFO: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
    } \
  }
  #define SCMULATE_INFOMSG_IF(level, condition, message, ...) { \
    if(VERBOSE_MODE >= level && condition) { \
      printf("[SCMULATE_INFO: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
    } \
  }
#else
  #define SCMULATE_WARNING(level, message, ...) {}
  #define SCMULATE_WARNING_IF(level, message, ...) {}
  #define SCMULATE_ERROR(level, message, ...) {}
  #define SCMULATE_ERROR_IF(level, message, ...) {}
  #define SCMULATE_INFOMSG(level, message, ...) {}
  #define SCMULATE_INFOMSG_IF(level, message, ...) {}
#endif // END IF VERBOSE_MODE


#endif // __SCMULATE_TOOLS__
