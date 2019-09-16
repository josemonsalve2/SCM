cmake_minimum_required(VERSION 2.8.9)
project (proof_of_concept)

include_directories(include)

# FLAGS FOR ALL CASES 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_STANDARD 11)


## CHECK FOR DEBUG MODE
if (DEBUG)
  set(CMAKE_BUILD_TYPE Debug)
else ()
  set(CMAKE_BUILD_TYPE Release)
endif (DEBUG)

## CHECK FOR VERBOSE EXECUTION MODE
if (VERBOSE_MODE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVERBOSE_MODE=${VERBOSE_MODE}")
endif (VERBOSE_MODE)

## COMPILER SPECIFIC
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qopenmp")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  message( FATAL_ERROR "No support for ${CMAKE_C_COMPILER_ID} yet" )
else()
  message( FATAL_ERROR "No support for ${CMAKE_C_COMPILER_ID} yet" )
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

add_subdirectory (src) 

add_executable(proof_of_concept proof_of_concept.c)

target_link_libraries (proof_of_concept registers)
