configure_file(matMul.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(matMulUnrroli.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

set(BLA_VENDOR Intel10_64ilp_seq)
find_package(BLAS)
if (NOT BLAS_FOUND)
message("Could not find MKL")
    set(BLA_VENDOR OpenBLAS)
    find_package(BLAS)
    if (NOT BLAS_FOUND)
        message( FATAL_ERROR "Could not find OpenBLAS either!")
    else()
        add_definitions("-DBLAS")
    endif()
else()
    add_definitions("-DMKL" "-DMKL_ILP64")
endif()

add_subdirectory(Codelets)

add_executable(MatMulX mainMatMul.cpp)
target_include_directories(MatMulX PRIVATE Codelets)

if (DECLARE_VARIANT_ENABLED)
  target_link_libraries (MatMulX scm_machine mat_mul_ofl_codX mat_mul_codX scm_system_codelets ${BLAS_LIBRARIES})
else ()
  target_link_libraries (MatMulX scm_machine mat_mul_codX scm_system_codelets ${BLAS_LIBRARIES})
endif()
    
