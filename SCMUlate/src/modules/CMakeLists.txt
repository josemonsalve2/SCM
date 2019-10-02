# REGISTERS
set( reg_src registers.cpp )
set( reg_inc 
    ${CMAKE_SOURCE_DIR}/include/modules/register.hpp
    ${CMAKE_SOURCE_DIR}/include/common/SCMUlate_tools.hpp)

add_library(registers ${reg_src} ${reg_inc})

# INSTRUCTION MEMORY
set( instruction_mem_src instruction_mem.cpp )
set( instruction_mem_inc 
    ${CMAKE_SOURCE_DIR}/include/modules/instruction_mem.hpp
    ${CMAKE_SOURCE_DIR}/include/common/SCMUlate_tools.hpp)

add_library(instruction_mem ${instruction_mem_src} ${instruction_mem_inc})

# FETCH_DECODE
set( fetch_decode_src fetch_decode.cpp )
set( fetch_decode_inc
    ${CMAKE_SOURCE_DIR}/include/modules/register.hpp
    ${CMAKE_SOURCE_DIR}/include/common/SCMUlate_tools.hpp
    ${CMAKE_SOURCE_DIR}/include/modules/fetch_decode.hpp)

add_library(fetch_decode ${fetch_decode_src} ${fetch_decode_inc})

