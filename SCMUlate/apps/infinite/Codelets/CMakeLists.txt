set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=skylake")
# MAT_MUL CODELET
set( sleep_cod_src sleep.cpp )
set( sleep_cod_inc
    ${CMAKE_SOURCE_DIR}/apps/infinite/Codelets/sleep.hpp )

add_library(sleep_cod ${sleep_cod_src} ${sleep_cod_inc})

target_link_libraries(sleep_cod scm_codelet ${BLAS_LIBRARIES})
target_sources(sleep_cod INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${sleep_cod_src})

