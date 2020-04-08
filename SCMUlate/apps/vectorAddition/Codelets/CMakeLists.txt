# VECT_ADD CODELET
set( vect_add_src vecAdd.cpp )
set( vect_add_inc
    ${CMAKE_SOURCE_DIR}/apps/vectorAddition/Codelets/vecAdd.hpp )

add_library(vect_add_codelet ${vect_add_src} ${vect_add_inc})
target_link_libraries(vect_add_codelet scm_codelet)
target_sources(vect_add_codelet INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${vect_add_src})