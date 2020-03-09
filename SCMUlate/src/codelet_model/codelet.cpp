#include "codelet.hpp"
#include "instructions.hpp"

scm::codelet* 
  scm::codeletFactory::createCodelet(std::string name, void * usedParams) {
    codelet * result;
    // Look for the codelet in the map
    auto found = registeredCodelets.find(name);
    if ( found == registeredCodelets.end()) {
      // If not found, we display error, and then return null.
      SCMULATE_ERROR(0,"Trying to create a codelet that has not been implemented");
      return nullptr;
    } else {
      // If found we call the creator function and then 
      SCMULATE_INFOMSG(5, "Creating Codelet %s", name.c_str());
      creatorFnc creator = found->second;
      result = creator(usedParams); // It is a function pointer
      return result;
    }
};

void 
  scm::codeletFactory::registerCreator(std::string name, scm::creatorFnc creator ){
    SCMULATE_INFOMSG(3, "Registering codelet %s", name.c_str());
    registeredCodelets[name] = creator;
  }


