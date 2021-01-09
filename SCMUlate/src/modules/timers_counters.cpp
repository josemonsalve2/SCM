#include "timers_counters.hpp"

namespace scm
{

void timers_counters::addTimer(std::string counterName, counter_type type)
{
  this->counters[counterName] = std::vector<timer_event>();
  this->counters[counterName].reserve(1e6);
  this->counterType[counterName] = type;
}

double timers_counters::getTimestamp() {
  std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - this->globalInitialTimer;
  return diff.count();
}
timer_event& timers_counters::addEvent(std::string counterName, int newEvent, std::string desc)
{
  timer_event event(newEvent, desc);
  this->counters.at(counterName).push_back(event);
  return this->counters.at(counterName).back();
}

void timers_counters::dumpTimers()
{
  // Needed for indentation
  std::string indent = "";
  const std::string indent_increment = "  ";
  auto indent_push = [&]() {
    indent += indent_increment;
  };
  auto indent_pop = [&]() {
    indent.erase(indent.length() - indent_increment.length(), indent_increment.length());
  };
  if (this->dumpFilename.length() == 0) {
    std::cout << "{\n";
    indent_push();
    int numElements = counters.size();
    // for each timer 
    for (std::pair<std::string, std::vector<timer_event>> element : counters)
    {
      numElements--;
      std::cout << indent << "\"" << element.first << "\": {\n";
      indent_push();

      std::cout << indent << "\"counter type\": \"" << std::to_string(this->counterType[element.first]) << "\",\n";
      std::cout << indent << "\"events\": [\n";
      indent_push();
      // For each event in the timer.
      for (timer_event& event : element.second)
      {
        std::cout << indent << "{\n";
        indent_push();
        std::cout << indent << "\"type\": \"" << std::to_string(event.getID()) << "\",\n";
        // Get the event timer value 
        std::chrono::duration<double> diff = event.getValue() - this->globalInitialTimer;
        std::cout << indent << "\"value\": " << std::to_string(diff.count()) << ",\n";
        #ifdef PAPI_COUNT
          std::vector <long long> PAPIvals = event.getPAPIcounters();
          if (PAPIvals.size() > 0) {
            std::cout << indent << "\"description\": \"" << event.getDescription() << "\",\n";
            std::cout << indent << "\"hw_counters\": {\n";
            indent_push();
            size_t papi_name_count = 0;
            for (auto vals_it = PAPIvals.begin(); vals_it != PAPIvals.end(); vals_it++, papi_name_count++) {
              if (papi_name_count != PAPIvals.size()-1) {
                std::cout << indent << "\""<< this->PAPI_events_str[papi_name_count]<<"\": "<< static_cast<unsigned long long>(*vals_it) << ", \n";
              } else {
                std::cout << indent << "\""<< this->PAPI_events_str[papi_name_count]<<"\": "<< static_cast<unsigned long long>(*vals_it) << " \n";
              }
            }
            indent_pop();
            std::cout << indent << "}\n";
          } else {
            std::cout << indent << "\"description\": \"" << event.getDescription() << "\"\n";
          }
        #else 
          std::cout << indent << "\"description\": \"" << event.getDescription() << "\"\n";
        #endif
        indent_pop();
        if (event != element.second.back())
          std::cout << indent << "},\n";
        else
          std::cout << indent << "}\n";
      }
      indent_pop();
      std::cout << indent << "]\n";
      indent_pop();
      if (numElements != 0)
        std::cout << indent << "},\n";
      else
        std::cout << indent << "}\n";
    }
    indent_pop();
    std::cout << indent << "}\n";
  } else {
    auto logFile = std::ofstream(this->dumpFilename);
    logFile << "{\n";
    indent_push();
    int numElements = counters.size();
    // for each timer 
    for (std::pair<std::string, std::vector<timer_event>> element : counters)
    {
      numElements--;
      logFile << indent << "\"" << element.first << "\": {\n";
      indent_push();

      logFile << indent << "\"counter type\": \"" << std::to_string(this->counterType[element.first]) << "\",\n";
      logFile << indent << "\"events\": [\n";
      indent_push();
      // For each event in the timer.
      for (timer_event event : element.second)
      {
        logFile << indent << "{\n";
        indent_push();
        logFile << indent << "\"type\": \"" << std::to_string(event.getID()) << "\",\n";
        // Get the event timer value 
        std::chrono::duration<double> diff = event.getValue() - this->globalInitialTimer;

        logFile << indent << "\"value\": " << std::to_string(diff.count()) << ",\n";
        #ifdef PAPI_COUNT
          std::vector <long long> PAPIvals = event.getPAPIcounters();
          if (PAPIvals.size() > 0) {
            logFile << indent << "\"description\": \"" << event.getDescription() << "\",\n";
            logFile << indent << "\"hw_counters\": {\n";
            indent_push();
            size_t papi_name_count = 0;
            for (auto vals_it = PAPIvals.begin(); vals_it != PAPIvals.end(); vals_it++, papi_name_count++) {
              if (papi_name_count != PAPIvals.size()-1) {
                logFile << indent << "\""<< this->PAPI_events_str[papi_name_count]<<"\": "<< static_cast<unsigned long long>(*vals_it) << ", \n";
              } else {
                logFile << indent << "\""<< this->PAPI_events_str[papi_name_count]<<"\": "<< static_cast<unsigned long long>(*vals_it) << " \n";
              }
            }
            indent_pop();
            logFile << indent << "}\n";
          } else {
            logFile << indent << "\"description\": \"" << event.getDescription() << "\"\n";
          }
        #else 
          logFile << indent << "\"description\": \"" << event.getDescription() << "\"\n";
        #endif
        indent_pop();
        if (event != element.second.back())
          logFile << indent << "},\n";
        else
          logFile << indent << "}\n";
      }
      indent_pop();
      logFile << indent << "]\n";
      indent_pop();
      if (numElements != 0)
        logFile << indent << "},\n";
      else
        logFile << indent << "}\n";
    }
    indent_pop();
    logFile << indent << "}\n";
  }
}

} // namespace scm