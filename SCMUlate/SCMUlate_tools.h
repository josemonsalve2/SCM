//===------ SCMUlate_tools.h -----------------------------------===//
// 
// Different helper functions and macros for error handling and 
// other commonly used operations
// 
//===-----------------------------------------------------------===//
#ifndef __SCMULATE_TOOLS__
#define __SCMULATE_TOOLS__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Macro for output of information, warning and error messages
#ifdef VERBOSE_MODE
  #define SCMULATE_WARNING(message, ...) { \
    printf("[SCMULATE_WARNING: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
  }
  #define SCMULATE_WARNING_IF(condition, message, ...) { \
    if(condition) { \
      printf("[SCMULATE_WARNING: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
    } \
  }
  
  #define SCMULATE_ERROR(message, ...) { \
    fprintf(stderr, "[SCMULATE_ERROR: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
  }
  #define SCMULATE_ERROR_IF(condition, message, ...) { \
    if(condition) { \
      fprintf(stderr, "[SCMULATE_ERROR: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
    } \
  }
  
  #define SCMULATE_INFOMSG(message, ...) { \
    printf("[SCMULATE_INFO: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
  }
  #define SCMULATE_INFOMSG_IF(condition, message, ...) { \
    if(condition) { \
      printf("[SCMULATE_INFO: %s:%i] " message "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); \
    } \
  }
#else
  #define SCMULATE_WARNING(message, ...) {}
  #define SCMULATE_WARNING_IF(message, ...) {}
  #define SCMULATE_ERROR(message, ...) {}
  #define SCMULATE_ERROR_IF(message, ...) {}
  #define SCMULATE_INFOMSG(message, ...) {}
  #define SCMULATE_INFOMSG_IF(message, ...) {}
#endif // END IF VERBOSE_MODE


#endif // __SCMULATE_TOOLS__
