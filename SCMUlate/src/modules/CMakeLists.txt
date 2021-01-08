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
set( fetch_decode_src fetch_decode.cpp ilp_controller.cpp )
set( fetch_decode_inc
    ${CMAKE_SOURCE_DIR}/include/modules/fetch_decode.hpp
    ${CMAKE_SOURCE_DIR}/include/modules/ilp_controller.hpp)

add_library(fetch_decode ${fetch_decode_src} ${fetch_decode_inc})
if (PROFILER_INSTRUMENT)
    target_link_libraries(fetch_decode ittnotify)
endif(PROFILER_INSTRUMENT)

# EXECUTORS
set( executor_src executor.cpp )
set( executor_inc
    ${CMAKE_SOURCE_DIR}/include/modules/executor.hpp)
    

add_library(executor ${executor_src} ${executor_inc})

# MEMORY INTERFACE
set( memory_interface_src memory_interface.cpp )
set( memory_interface_inc
    ${CMAKE_SOURCE_DIR}/include/modules/memory_interface.hpp)
    

add_library(memory_interface ${memory_interface_src} ${memory_interface_inc})

# CONTROL_STORE
set( control_store_src control_store.cpp)
set( control_store_inc
    ${CMAKE_SOURCE_DIR}/include/modules/control_store.hpp)
    

add_library(control_store ${control_store_src} ${control_store_inc})

# TIMERS_COUNTERS
set( timers_counters_src timers_counters.cpp)
set( timers_counters_inc
    ${CMAKE_SOURCE_DIR}/include/modules/timers_counters.hpp)
    

add_library(scm_timers_counters ${timers_counters_src} ${timers_counters_inc})
