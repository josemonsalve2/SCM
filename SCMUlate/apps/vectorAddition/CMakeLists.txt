configure_file(vectorAddNoLoop.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(vectorAddLoop.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(vectorAddLoopUnrolled.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(vectorAddLoopUnrolled2.scm ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

add_subdirectory(Codelets)

add_executable(VecAdd mainVecAdd.cpp)
target_include_directories(VecAdd PRIVATE Codelets)

target_link_libraries (VecAdd scm_machine vect_add_codelet scm_system_codelets)
