#ifndef __TIMERS_COUNTERS__
#define __TIMERS_COUNTERS__
#include <map>
#include <string>
#include <vector> 
#include <ostream>
#include <iostream>
#include <fstream>
#include <chrono>

#ifdef TIMERS_COUNTERS
#define TIMERS_COUNTERS_GUARD(code) code
#else
#define TIMERS_COUNTERS_GUARD(code) /* code */
#endif

#ifdef PAPI_COUNT
#include "papi.h"
#endif

namespace scm {

  enum counter_type {
    SYS_TIMER,
    SU_TIMER, 
    MEM_TIMER, 
    CUMEM_TIMER
  };

  enum SYS_event {
    SYS_START,
    SYS_END
  };

  enum SU_event {
    SU_START,
    SU_END,
    FETCH_DECODE_INSTRUCTION,
    DISPATCH_INSTRUCTION, 
    EXECUTE_CONTROL_INSTRUCTION,
    EXECUTE_ARITH_INSTRUCTION,
    SU_IDLE
  };

  enum CUMEM_event {
    CUMEM_START, 
    CUMEM_END,
    CUMEM_EXECUTION_COD,
    CUMEM_EXECUTION_MEM,
    CUMEM_IDLE
  };

  class timer_event {
    private:
      int eventID;
      std::string description;
      std::chrono::time_point<std::chrono::high_resolution_clock> counter;
    public:
      timer_event(int id, std::string desc = std::string()) : eventID(id), description(desc), counter(std::chrono::high_resolution_clock::now()) { }
      inline int getID() { return this->eventID; }
      inline std::string getDescription() { return this->description; }
      inline std::chrono::time_point<std::chrono::high_resolution_clock> getValue() { return this->counter; }     
      inline bool operator==(const timer_event& other){ return (eventID == other.eventID && counter == other.counter); }
      bool operator != (const timer_event& other) { return eventID != other.eventID || counter != other.counter;  }
  };

  class timers_counters {
    private:
#ifdef PAPI_COUNT
    int PAPI_events[] = {
      PAPI_TOT_CYC,
      PAPI_L2_DCM,
      PAPI_L2_DCA };
#endif
      std::chrono::time_point<std::chrono::high_resolution_clock> globalInitialTimer;
      std::map <std::string, std::vector<timer_event>> counters;
      std::map <std::string, counter_type> counterType;
      int numCounters;
      std::string dumpFilename;

    public:
      timers_counters () : numCounters(0), dumpFilename("") {
        globalInitialTimer = std::chrono::high_resolution_clock::now();
        #ifdef PAPI_COUNT
          	PAPI_library_init(PAPI_VER_CURRENT);
        #endif
      }

      void resetTimer() { globalInitialTimer = std::chrono::high_resolution_clock::now(); }
      void addTimer(std::string, counter_type type);
      void addEvent(std::string, int, std::string = std::string());
      void dumpTimers();
      inline void setFilename(std::string fn) { dumpFilename = fn; }
  };
}
#endif