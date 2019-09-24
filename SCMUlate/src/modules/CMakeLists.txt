# REGISTERS
set( reg_src registers.cpp )
set( reg_inc 
    ${CMAKE_SOURCE_DIR}/include/modules/register.hpp
    ${CMAKE_SOURCE_DIR}/include/common/SCMUlate_tools.h)

add_library(registers ${reg_src} ${reg_inc})

# FETCH_DECODE
set( fetch_decode_src fetch_decode.cpp )
set( fetch_decode_inc
    ${CMAKE_SOURCE_DIR}/include/modules/register.hpp
    ${CMAKE_SOURCE_DIR}/include/common/SCMUlate_tools.hpp
    ${CMAKE_SOURCE_DIR}/include/modules/fetch_decode.hpp)

add_library(fetch_decode ${fetch_decode_src} ${fetch_decode_inc})

# SCM_MACHINE
set( scm_machine_src scm_machine.cpp )
set( scm_machine_inc
    ${CMAKE_SOURCE_DIR}/include/modules/scm_machine.hpp
    ${CMAKE_SOURCE_DIR}/include/common/SCMUlate_tools.hpp)

add_library(scm_machine ${scm_machine_src} ${scm_machine_inc})
