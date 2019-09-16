# REGISTERS
set( reg_src registers.c )
set( reg_inc 
    ${CMAKE_SOURCE_DIR}/include/register_configuration.h
    ${CMAKE_SOURCE_DIR}/include/SCMUlate_tools.h)

add_library(registers ${reg_src} ${reg_inc})
