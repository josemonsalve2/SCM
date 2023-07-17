configure_file(luDecomp.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(debug_luDecomp.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CLANG_CXX_FAMILY}" STREQUAL "icpx")
set(PREVIOUS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fiopenmp -fstack-protector-all -fstack-protector -march=skylake -fiopenmp -fopenmp-targets=spir64 -DDECLARE_VARIANT=1")
endif()

add_subdirectory(Codelets)

add_executable(LuDecomp mainLuDecomp.cpp)
target_include_directories(LuDecomp PRIVATE Codelets)
target_link_libraries (LuDecomp scm_machine lu_decomp_cod scm_system_codelets)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CLANG_CXX_FAMILY}" STREQUAL "icpx")
set(CMAKE_CXX_FLAGS "${PREVIOUS}")
set(CMAKE_CXX_LINK_FLAGS "")
endif()
