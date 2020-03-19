#include "timers_counters.hpp"
#include <math.h>

#define WASTE_TIME(aVar) for(int i = 0; i < 10000000; i++) pow(aVar,0.5)

int main () {
    volatile double aVar; 
  scm::timers_counters timers;

  //timers.setFilename("test.json");

  timers.addTimer("test1", scm::SYS_TIMER);
  timers.addTimer("test2", scm::SYS_TIMER);
  WASTE_TIME(aVar);
  timers.addEvent("test1", scm::timer_event(scm::SYS_START));
  WASTE_TIME(aVar);
  timers.addEvent("test1", scm::timer_event(scm::SYS_END));
  WASTE_TIME(aVar);
  timers.addEvent("test2", scm::timer_event(scm::SYS_START));
  WASTE_TIME(aVar);
  timers.addEvent("test2", scm::timer_event(scm::SYS_START));

  timers.dumpTimers();
  return 0;
}
