# SCM_MACHINE
set( scm_machine_src scm_machine.cpp )
set( scm_machine_inc
    ${CMAKE_SOURCE_DIR}/include/machines/scm_machine.hpp
    ${CMAKE_SOURCE_DIR}/include/common/SCMUlate_tools.hpp)

add_library(scm_machine ${scm_machine_src} ${scm_machine_inc})
target_link_libraries(scm_machine instruction_mem registers fetch_decode executor control_store memory_interface scm_codelet scm_system_codelets)
target_compile_options(scm_machine PRIVATE -fopenmp)
