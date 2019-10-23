# Modules folder

This folder contains the modules that could be used to create a machine. Most likely as the complexity of the machines increase, we will divide this folder in different versions, and in base classes.

# Files

*  **control_store.hpp:** This module corresponds to the logic that connects a particular codelet with its possible executor
*  **executor.hpp:** This module corresponds to the logic that the executor uses. It represents the program the executor thread runs while either waiting for work or executing a Codelet
*  **fetch_decode.hpp:** This module does the fetch and decode of instructions from memory. It does not have the memory itself, but a reference to the memory, and keeps track of the current program counter. 
*  **instruction_mem.hpp:** This corresponds to the instruction memory. This also includes the logic needed to read from the file that has the executing program, and place that file into memory.
*  **register_config.hpp:** This corresponds to the macros and logic to be able to split the Cache into a virtual register file. This file is experimental for now and it has been adapted to cover my ANL system
*  **register.hpp:** This is the actual register handling, and needed logic to interact with the register file
