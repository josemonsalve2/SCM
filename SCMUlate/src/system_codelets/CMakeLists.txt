# SCM_CODELET
set( system_codelets_src cod_print.cpp )
set( system_codelets_inc
    ${CMAKE_SOURCE_DIR}/include/system_codelets/system_codelets.hpp )

add_library(scm_system_codelets ${system_codelets_src} ${system_codelets_inc})