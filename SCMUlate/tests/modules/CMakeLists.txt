# Test for REGISTER FILE
set (test_reg_file_src test_register_file.cpp)
set (test_reg_file_inc 
      ${CMAKE_SOURCE_DIR}/include/modules/register.hpp)

add_executable(test_reg_file ${test_reg_file_src} ${test_reg_file_inc})
target_link_libraries(test_reg_file registers)

add_test(NAME test_reg_file COMMAND test_reg_file WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Test for INSTRUCTION MEMORY
set (test_inst_mem_src test_instruction_mem.cpp)
set (test_inst_mem_inc 
      ${CMAKE_SOURCE_DIR}/include/modules/instruction_mem.hpp)

add_executable(test_inst_mem ${test_inst_mem_src} ${test_inst_mem_inc})
target_link_libraries(test_inst_mem instruction_mem)
configure_file(test_mem_file.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

add_test(NAME test_inst_mem COMMAND test_inst_mem  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
