#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=skylake")
# MAT_MUL CODELET
set( mat_mul_cod_src MatMul.cpp )
set( mat_mul_cod_inc
    ${CMAKE_SOURCE_DIR}/apps/matrixMult/Codelets/MatMul.hpp )

add_library(mat_mul_cod ${mat_mul_cod_src} ${mat_mul_cod_inc})

target_link_libraries(mat_mul_cod scm_codelet ${BLAS_LIBRARIES})
target_sources(mat_mul_cod INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${mat_mul_cod_src})

# MAT_MUL OFFLOADING CODELET
if (DECLARE_VARIANT_ENABLED)
  message("Compiling MatMul Offloading with declare variant intel's extension")
  set( mat_mul_omp_ofl_cod_src MatMulOMPoffload.cpp )
  set( mat_mul_omp_ofl_cod_inc
      ${CMAKE_SOURCE_DIR}/apps/matrixMult/Codelets/MatMulOMPoffload.hpp )
  
  add_library(mat_mul_ofl_cod ${mat_mul_omp_ofl_cod_src} ${mat_mul_omp_ofl_cod_inc})
  
  target_link_libraries (mat_mul_ofl_cod scm_codelet ${BLAS_LIBRARIES})
  target_sources (mat_mul_ofl_cod INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${mat_mul_omp_ofl_cod_src})
endif()
