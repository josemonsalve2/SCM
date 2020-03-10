# SCM_MACHINE
set( scm_string_helper_src stringHelper.cpp )
set( scm_string_helper_inc
    ${CMAKE_SOURCE_DIR}/include/common/stringHelper.hpp)

add_library(scm_string_helper ${scm_string_helper_src} ${scm_string_helper_inc})
