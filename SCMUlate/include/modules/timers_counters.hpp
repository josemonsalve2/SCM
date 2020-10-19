#ifndef __TIMERS_COUNTERS__
#define __TIMERS_COUNTERS__
#include <map>
#include <string>
#include <vector> 
#include <ostream>
#include <iostream>
#include <fstream>
#include <chrono>
#include "SCMUlate_tools.hpp"
#include "omp.h"

#ifdef TIMERS_COUNTERS
#define TIMERS_COUNTERS_GUARD(code) code
#else
#define TIMERS_COUNTERS_GUARD(code) /* code */
#endif

#ifdef PAPI_COUNT
#include "papi.h"


#define PAPI_EVENTS PAPI_TOT_CYC, PAPI_DP_OPS, PAPI_DP_OPS
//#define PAPI_EVENTS PAPI_L3_TCM, PAPI_LST_INS, PAPI_L3_TCA
//#define PAPI_EVENTS PAPI_TOT_CYC, PAPI_RES_STL
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

#ifdef PAPI_COUNT
  class papi_counters_t {
    private:
      int EventSet;
      int numEvents;
      long long *values;
    public:
      papi_counters_t(std::vector<int> &events, std::vector<std::string> &eventNames) : EventSet(PAPI_NULL), numEvents(0), values(nullptr) {
        // Create an event set and add the different events. 
        // Start meassuring 
        int retVal; 
        retVal = PAPI_create_eventset(&EventSet);
        if (retVal != PAPI_OK) {
          SCMULATE_ERROR(0, "PAPI Error starting the EventSet");
          return;
        }
        auto it_event = events.begin();
        auto it_name = eventNames.begin();
        for (; it_event != events.end(); it_event++, it_name++) {
          retVal = PAPI_add_event(EventSet, *it_event);
          SCMULATE_INFOMSG(4,"PAPI, Adding event to event %s to Evenset %d", it_name->c_str(), EventSet);
          if (retVal != PAPI_OK) {
            SCMULATE_ERROR(0, "PAPI, error adding the Event %s to Evenset %d", it_name->c_str(), EventSet);
            continue;
          }
          numEvents++;
        }
        // Allocate space for the values
        values = new long long [numEvents];
      }

      void startCounting () {
        if (EventSet != PAPI_NULL) {
          PAPI_reset(EventSet);
          PAPI_start(EventSet);
        }
      }

      std::vector <long long> getCurrentValues() {
        if (EventSet != PAPI_NULL) {
          PAPI_read(EventSet, values);
          return getValues();
        }
        return std::vector<long long>(); 
      }

      std::vector <long long> getValues () {
        std::vector<long long> retValues;
        for (int i = 0; i < numEvents; i++)
          retValues.push_back(values[i]);
        return retValues;
      }

      void stopCounting() {
        if (EventSet != PAPI_NULL) {
          PAPI_stop(EventSet, values);
          // int status;
          // PAPI_state(EventSet, &status);
          // if (status == PAPI_OVERFLOWING) {
          //   printf("OVERFLOWWWW\n");
          // }
          PAPI_reset(EventSet);

        }
      }

      ~papi_counters_t() {
        // Delete the event set and stop counters
        if (EventSet != PAPI_NULL) {
          PAPI_destroy_eventset(&EventSet);
          delete []values;
        }
      }
  };
#endif 

  class timer_event {
    private:
      int eventID;
      std::string description;
      std::chrono::time_point<std::chrono::high_resolution_clock> counter;
      #ifdef PAPI_COUNT
        std::vector<long long> PAPICounters;
      #endif
    public:
      timer_event(int id, std::string desc = std::string()) : eventID(id), description(desc), counter(std::chrono::high_resolution_clock::now()) { }
      timer_event(const timer_event& other) {
        this->eventID = other.eventID;
        this->description = other.description;
        this->counter = other.counter;
        #ifdef PAPI_COUNT
          this->PAPICounters = other.PAPICounters;
        #endif
      }
      inline int getID() { return this->eventID; }
      inline std::string getDescription() { return this->description; }
      inline std::chrono::time_point<std::chrono::high_resolution_clock> getValue() { return this->counter; }
      #ifdef PAPI_COUNT
        inline void addPAPIcounters(std::vector<long long> counts) { PAPICounters = counts; }
        inline std::vector<long long>& getPAPIcounters() { return PAPICounters; }
      #endif
      inline bool operator==(const timer_event& other){ return (eventID == other.eventID && counter == other.counter); }
      bool operator != (const timer_event& other) { return eventID != other.eventID || counter != other.counter;  }
  };

  class timers_counters {
    private:
#ifdef PAPI_COUNT
      std::vector<int> PAPI_events;
      std::vector<std::string> PAPI_events_str;
      std::map <std::string, papi_counters_t*> PAPIcountersManager;
      void init_papi_events() {}
      template <typename ...Args>
      void init_papi_events(int cur_val, Args... args) {
        PAPI_events.push_back(cur_val);
        char event_name[PAPI_MAX_STR_LEN];
        int retVal = PAPI_event_code_to_name(cur_val, event_name);
        if (retVal != PAPI_OK)
          SCMULATE_ERROR(0, "PAPI error on finding event name");
        SCMULATE_INFOMSG(4, "PAPI, Registering counter %s", event_name);
        std::string event_name_str(event_name);
        PAPI_events_str.push_back(event_name_str);
        init_papi_events(args...);
      }
      const PAPI_hw_info_t *hw_info = NULL;
      
      std::map <std::string, papi_counters_t> papi_counters;
      bool disablePapi;

#endif
      std::chrono::time_point<std::chrono::high_resolution_clock> globalInitialTimer;
      std::map <std::string, std::vector<timer_event>> counters;
      std::map <std::string, counter_type> counterType;
      std::string dumpFilename;
      static unsigned long omp_get_thread_num_wrapper(void) {
        return (unsigned long)omp_get_thread_num();
      }

    public:
      timers_counters () : dumpFilename("") {
        globalInitialTimer = std::chrono::high_resolution_clock::now();
        #ifdef PAPI_COUNT
          disablePapi = true;
          init_papi_events(PAPI_EVENTS);
          if( PAPI_library_init( PAPI_VER_CURRENT ) == PAPI_VER_CURRENT ) {
            disablePapi = false;
            SCMULATE_INFOMSG(3, "Your PAPI has been initialized!");
          } else {
             SCMULATE_ERROR(0, "PAPI! I could not load.");
          }
          hw_info = PAPI_get_hardware_info();
          SCMULATE_ERROR_IF(0, hw_info == NULL, "PAPI Could not get your HW info!");
          int i=0;
          for (auto it = PAPI_events.begin(); it != PAPI_events.end(); it++, i++) {
            if (PAPI_query_event(*it) != PAPI_OK)
              SCMULATE_ERROR(0, "PAPI! Error with missing event i = %s", PAPI_events_str[i].c_str());
          }
          int retval = PAPI_thread_init( omp_get_thread_num_wrapper);
          if (retval != PAPI_OK) {
            SCMULATE_ERROR(0, "PAPI! Problems with your threads")
            SCMULATE_ERROR_IF(0, retval != PAPI_ECMP , "PAPI! Trouble with the threads")
          }
        #endif
      }

      #ifdef PAPI_COUNT
        void initPAPIcounter(std::string counterName) {
          this->PAPIcountersManager[counterName] = new papi_counters_t(this->PAPI_events, this->PAPI_events_str);
        }
        void startPAPIcounters(std::string counterName) {
          this->PAPIcountersManager[counterName]->startCounting();
        }
        void stopAndRegisterPAPIcounters(std::string counterName, timer_event &event) {
          this->PAPIcountersManager[counterName]->stopCounting();
          event.addPAPIcounters(this->PAPIcountersManager[counterName]->getValues());
        }
      #endif

      void resetTimer() { 
        globalInitialTimer = std::chrono::high_resolution_clock::now(); 
      }
      void addTimer(std::string, counter_type type);
      timer_event& addEvent(std::string, int, std::string = std::string());
      void dumpTimers();
      inline void setFilename(std::string fn) { dumpFilename = fn; }
      ~timers_counters() {
        #ifdef PAPI_COUNT
          PAPI_unregister_thread();
          for(std::pair<std::string,papi_counters_t*> manager :  PAPIcountersManager) {
            delete manager.second;
          }
        #endif
      }
  };
}
#endif