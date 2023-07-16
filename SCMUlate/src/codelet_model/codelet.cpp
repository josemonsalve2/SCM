#include "codelet.hpp"
#include "instructions.hpp"
#include "executor.hpp"

namespace scm {

std::map<std::string, creatorFnc> *codeletFactory::registeredCodelets;
static int codeletFactoryInit;

l2_memory_t 
  codelet::getAddress(uint64_t addr) {
   return this->getExecutor()->get_mem_interface()->getAddress(addr);
}
l2_memory_t 
  codelet::getAddress(l2_memory_t addr) {
   return this->getExecutor()->get_mem_interface()->getAddress(addr);
}

codelet *codeletFactory::createCodelet(std::string name,
                                       codelet_params &usedParams) {
   codelet *result;
   // Look for the codelet in the map
   auto found = registeredCodelets->find(name);
   if (found == registeredCodelets->end()) {
     // If not found, we display error, and then return null.
     SCMULATE_ERROR(0,
                    "Trying to create a codelet that has not been implemented");
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
  codeletFactory::registerCreator(std::string name, creatorFnc creator ) {
    if (codeletFactoryInit++ == 0) registeredCodelets = new std::map<std::string, creatorFnc>();
    SCMULATE_INFOMSG(3, "Registering codelet %s", name.c_str());
    (*registeredCodelets)[name] = creator;
  }
}