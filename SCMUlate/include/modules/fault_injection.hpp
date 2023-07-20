#pragma once

#include "SCMUlate_tools.hpp"
#include "instructions.hpp"
#include "system_config.hpp"
#include <math.h>
#include <string>
#include <chrono>

#define DEFAULT_BETTA 0.5
#define DEFAULT_LAMBDA 0.005

namespace scm {

class FaultInjection {
private:
  FAULT_INJECTION_MODES mode;
  std::chrono::_V2::system_clock::time_point last_error_time;

  double betta = DEFAULT_BETTA;
  double lambda = DEFAULT_LAMBDA;

  // Represents F(t) the probability that the component will fail at or before
  // time t
  double getCummulativePoisson(double x) { return 1 - exp(-lambda * x); }

  // Find the next time where the probability is x in a poisson distribution
  double getPoissonNextErrorTime(double x) { return -log(1 - x) / lambda; }

  // Represents F(t) the probability that the component will fail at or before
  // time t
  double getCummulativeWeibull(double x) {
    return 1 - exp(-lambda * pow(x, betta));
  }

  // Find the next time where the probability is x in a weibull distribution
  double getWeibullNextErrorTime(double x) {
    return pow(-log(1 - x) / lambda, 1 / betta);
  }

public:
  FaultInjection() : mode(FAULT_INJECTION_MODES::NO_FAULT_INJECTION),
    last_error_time(std::chrono::high_resolution_clock::now()) {
    // Check if SCM_FAULT_INJECTION_MODE is set in the environment
    // If so, override the mode
    char *env = getenv("SCM_FAULT_INJECTION_MODE");
    if (env != NULL) {
      std::string env_str(env);
      if (env_str == "NO_FAULT_INJECTION") {
        mode = NO_FAULT_INJECTION;
      } else if (env_str == "POISSON_FAULT_INJECTION") {
        mode = POISSON_FAULT_INJECTION;
      } else if (env_str == "WEIBULL_FAULT_INJECTION") {
        mode = WEIBULL_FAULT_INJECTION;
      } else {
        std::string options("NO_FAULT_INJECTION, POISSON_FAULT_INJECTION, "
                            "WEIBULL_FAULT_INJECTION");
        SCMULATE_ERROR(0, "Invalid Fault Injection Mode: %s, options are %s",
                       env_str.c_str(), options.c_str());
      }
    }
    // Check for SCM_WEIBULL_BETTA and SCM_LAMBDA
    // If so, override the values
    env = getenv("SCM_WEIBULL_BETTA");
    if (env != NULL)
      betta = std::stod(env);
    env = getenv("SCM_LAMBDA");
    if (env != NULL)
      lambda = std::stod(env);
  }

  bool shouldInjectFault(double t_ms) {
    switch (mode) {
    case NO_FAULT_INJECTION:
      return false;
    case POISSON_FAULT_INJECTION:
      SCMULATE_INFOMSG(
          6, "Poisson Fault. Probability of Injection %f, execution time %f",
          getCummulativePoisson(t_ms), t_ms);
      return getCummulativePoisson(t_ms) > (double)rand() / RAND_MAX;
    case WEIBULL_FAULT_INJECTION:
      SCMULATE_INFOMSG(6,
                       "Weibull Fault. Probability of Injection %f "
                       "execution time %f",
                       getCummulativeWeibull(t_ms), t_ms);
      return getCummulativeWeibull(t_ms) > (double)rand() / RAND_MAX;
    default:
      SCMULATE_ERROR(0, "Invalid Fault Injection Mode: %s",
                     FAULT_INJECTION_MODES_STR[mode]);
    }
  }

  void injectFault(instruction_state_pair* inst) {
    auto current_time = std::chrono::high_resolution_clock::now();
    // End timer in ms
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_error_time);

    auto t_ms = duration.count();

    if (shouldInjectFault(t_ms)) {
      last_error_time = current_time;
      if (inst == nullptr)
        return;
      inst->first->setItFailed(true);
#if REGISTER_INJECTION_MODE == 1
      // Iterate over the operands, search for write and readwrite
      std::unordered_map<decoded_reg_t, reg_state> *inst_operand_dir =
          inst->first->getOperandsDirs();
      // Iterage over operands and scramble write registers
      for (int i = 1; i <= MAX_NUM_OPERANDS; ++i) {
        operand_t *current_operand = &inst->first->getOp(i);
        if (current_operand->type == operand_t::REGISTER) {
          auto it_inst_dir = inst_operand_dir->find(current_operand->value.reg);
          if (it_inst_dir->second == reg_state::WRITE ||
              it_inst_dir->second == reg_state::READWRITE) {
            decoded_reg_t &reg = current_operand->value.reg;
            for (uint32_t j = 0; j < reg.reg_size_bytes; ++j)
              reg.reg_ptr[j] = rand() % 256;
          }
        }
      }
#endif
      SCMULATE_INFOMSG(2, "Fault injected to %s at time %f",
                       inst->first->getFullInstruction().c_str(), t_ms);
    }
  }
};

} // namespace scm