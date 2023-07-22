configure_file(infinite.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CLANG_CXX_FAMILY}" STREQUAL "icpx")
set(PREVIOUS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -march=skylake")
endif()

add_subdirectory(Codelets)

add_executable(inifinite mainInfinite.cpp)
target_include_directories(inifinite PRIVATE Codelets)

target_link_libraries (inifinite scm_machine sleep_cod scm_system_codelets)


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CLANG_CXX_FAMILY}" STREQUAL "icpx")
set(CMAKE_CXX_FLAGS "${PREVIOUS}")
set(CMAKE_CXX_LINK_FLAGS "")
endif()
