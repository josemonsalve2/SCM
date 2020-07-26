# List of unimplemented things

* SHFL and SHFR operations
* MULT Operation
* Signed support for the immediate values
* Support for more than three operands in a Codelet
* A better memory allocation mechanism for the machine:
    * L3 Memory, L3 Malloc for the outer program?
* Better understand how to pass parameters within the Codelet:
    * Parameters change the latency of a Codelet. I/O operations may trickle down the hierarchy
* JMPLABEL may me more efficient if it can be pre-decoded. However, the current method keeps the value in a map and find the jump offset at runtime