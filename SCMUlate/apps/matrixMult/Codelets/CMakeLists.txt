# VECT_ADD CODELET
set( mat_mul_cod_src MatMul.cpp )
set( mat_mul_cod_inc
    ${CMAKE_SOURCE_DIR}/apps/matrixMult/Codelets/MatMul.hpp )

add_library(mat_mul_cod ${mat_mul_cod_src} ${mat_mul_cod_inc})

target_link_libraries(mat_mul_cod scm_codelet ${BLAS_LIBRARIES})
target_sources(mat_mul_cod INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${mat_mul_cod_src})