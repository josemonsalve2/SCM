#include "timers_counters.hpp"
#include <math.h>

#define WASTE_TIME(aVar) for(int i = 0; i < 10000000; i++) pow(aVar,0.5)

int main () {
    volatile double aVar = 1; 
  scm::timers_counters timers;

  //timers.setFilename("test.json");

  timers.addTimer("test1", scm::SYS_TIMER);
  timers.addTimer("test2", scm::SYS_TIMER);
  WASTE_TIME(aVar);
  timers.addEvent("test1", scm::SYS_START, "test1");
  WASTE_TIME(aVar);
  timers.addEvent("test1", scm::SYS_END, "test1");
  WASTE_TIME(aVar);
  timers.addEvent("test2", scm::SYS_START, "test2");
  WASTE_TIME(aVar);
  timers.addEvent("test2", scm::SYS_START, "test2");

  timers.dumpTimers();
  return 0;
}
