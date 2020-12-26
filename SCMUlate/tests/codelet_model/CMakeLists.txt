message(" -> Codelet Model:")


# Test for REGISTER FILE
set (test_codelet_src test_codelet.cpp)
set (test_codelet_inc 
      ${CMAKE_SOURCE_DIR}/include/codelet_model/codelet.hpp)

add_executable(test_codelet ${test_codelet_src} ${test_codelet_inc})
target_link_libraries(test_codelet scm_codelet)

add_test(NAME test_codelet COMMAND test_codelet WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
