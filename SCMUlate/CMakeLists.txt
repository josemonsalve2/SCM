cmake_minimum_required(VERSION 2.8.9)
project (proof_of_concept)

include_directories(include/common)
include_directories(include/modules)
include_directories(include/machines)
include_directories(include/codelet_model)
include_directories(include/system_codelets)

# FLAGS FOR ALL CASES 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_STANDARD 17)


## CHECK FOR DEBUG MODE
if (DEBUG)
  set(CMAKE_BUILD_TYPE Debug)
else ()
  set(CMAKE_BUILD_TYPE Release)
endif (DEBUG)

## CHECK FOR VERBOSE EXECUTION MODE
if (VERBOSE_MODE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVERBOSE_MODE=${VERBOSE_MODE}")
endif (VERBOSE_MODE)

## CHECK FOR VERBOSE EXECUTION MODE
if (TIMERS_COUNTERS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTIMERS_COUNTERS=1")
endif (TIMERS_COUNTERS)

## OS Specific
if (APPLE)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    include_directories("/usr/local/include" "/usr/local/opt/llvm/include")
    link_directories("/usr/local/lib" "/usr/local/opt/llvm/lib")
    set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
  endif()
endif (APPLE)

## COMPILER SPECIFIC
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  get_filename_component(CLANG_CXX_FAMILY ${CMAKE_CXX_COMPILER} NAME)
  message("Identified Clang family as ${CLANG_CXX_FAMILY}")
  # using Clang
  if ("${CLANG_CXX_FAMILY}" STREQUAL "icpx")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fiopenmp ")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopenmp")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  message( FATAL_ERROR "No support for ${CMAKE_CXX_COMPILER_ID} yet" )
else()
  message( FATAL_ERROR "No support for ${CMAKE_CXX_COMPILER_ID} yet" )
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O3 -std=c++17")

add_subdirectory (src) 
if (CMAKE_BUILD_TYPE MATCHES Debug)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    if (APPLE)
      message("Adding sanitizer for Clang in Apple")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    elseif(UNIX)
      message("Adding sanitizer for Clang in Linux")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
    endif()

  endif()
  enable_testing()
  add_subdirectory (tests) 
endif()

add_subdirectory(apps)

add_executable(SCMUlate main_SCMUlate.cpp)

target_link_libraries (SCMUlate scm_machine scm_system_codelets)
