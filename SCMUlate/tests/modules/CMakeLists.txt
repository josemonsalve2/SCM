set (test_inst_mem_src test_instruction_mem.cpp)
set (test_inst_mem_inc 
      ${CMAKE_SOURCE_DIR}/include/modules/instruction_mem.hpp)

add_executable(test_inst_mem ${test_inst_mem_src} ${test_inst_mem_inc})
target_link_libraries(test_inst_mem instruction_mem)
configure_file(test_mem_file.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

add_test(NAME test_inst_mem COMMAND test_inst_mem  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
