cmake_minimum_required(VERSION 2.8.9)
project (proof_of_concept)

include_directories(include/common)
include_directories(include/modules)
include_directories(include/machines)
include_directories(include/codelet_model)
include_directories(include/system_codelets)

# FLAGS FOR ALL CASES 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_STANDARD 14)


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

## COMPILER SPECIFIC
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_subdirectory (src) 
if (CMAKE_BUILD_TYPE MATCHES Debug)
  enable_testing()
  add_subdirectory (tests) 
endif()

add_subdirectory(apps)

add_executable(SCMUlate main_SCMUlate.cpp)

target_link_libraries (SCMUlate scm_machine)
