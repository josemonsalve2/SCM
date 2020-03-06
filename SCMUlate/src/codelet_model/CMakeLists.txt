# SCM_CODELET
set( scm_codelet_src codelet.cpp )
set( scm_codelet_inc
    ${CMAKE_SOURCE_DIR}/include/codelet_model/codelet.hpp )

add_library(scm_codelet ${scm_codelet_src} ${scm_codelet_inc})