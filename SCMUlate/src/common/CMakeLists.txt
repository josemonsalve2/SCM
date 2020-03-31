# STRING_HELPER
set( scm_string_helper_src stringHelper.cpp )
set( scm_string_helper_inc
    ${CMAKE_SOURCE_DIR}/include/common/stringHelper.hpp)

add_library(scm_string_helper ${scm_string_helper_src} ${scm_string_helper_inc})

# INSTRUCTIONS
set( scm_instructions_src instructions.cpp )
set( scm_instructions_inc
    ${CMAKE_SOURCE_DIR}/include/common/instructions.hpp)
    
add_library(scm_instructions ${scm_instructions_src} ${scm_instructions_inc})
target_link_libraries(scm_instructions scm_codelet)
