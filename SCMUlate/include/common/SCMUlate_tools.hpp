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

typedef unsigned char * l2_memory_t;

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#ifndef VERBOSE_MODE
#define VERBOSE_MODE -1
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
    } \
  }
  #define SCMULATE_ERROR_IF(level, condition, message, ...) { \
    if(VERBOSE_MODE >= level && condition) { \
      fprintf(stderr, "[SCMULATE_ERROR: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
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
