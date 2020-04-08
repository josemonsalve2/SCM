# SYSTEM_CODELET
set( system_codelets_src cod_print.cpp )
set( system_codelets_inc
    ${CMAKE_SOURCE_DIR}/include/system_codelets/system_codelets.hpp
    ${CMAKE_SOURCE_DIR}/include/system_codelets/cod_print.hpp )

add_library(scm_system_codelets ${system_codelets_src} ${system_codelets_inc})
target_link_libraries(scm_system_codelets scm_codelet)
target_sources(scm_system_codelets INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${system_codelets_src})