#ifndef __CODELET__
#define __CODELET__

namespace scm {
  typedef void (*execution_fnc)();

  class codelet {
    private:
      execution_fnc myFnc;
  
    public:
      codelet() = delete;
      codelet(execution_fnc fnc) : myFnc(fnc) {}
  }
  
}

#endif
