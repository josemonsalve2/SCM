#ifndef __DUPL_CONTROLLER__
#define __DUPL_CONTROLLER__

/** \brief Codelet duplicator
 *
 * This class is responsible for duplicating codelets
 * There are different strategies that are implemented. Each will
 */

#include "SCMUlate_tools.hpp"
#include "instruction_buffer.hpp"
#include "instructions.hpp"
#include "system_config.hpp"
#include <unordered_map>
#include <unordered_set>

constexpr int MAX_DUPLICATED_CODELETS = 10;

namespace scm {

class fetch_decode_module;

class dupl_controller_module {
private:
  DUPL_MODES mode;

  // Hidden register file for duplication
  reg_file_module hidden_reg_file_m;

  instructions_buffer_module *inst_buff_m;
  fetch_decode_module *fetch_decode_m;

  std::unordered_map<decoded_reg_t, decoded_reg_t> originalVals;
  std::unordered_set<unsigned char *> renamedInUse;

  decoded_reg_t getRenamedRegister(decoded_reg_t &otherReg);

  decoded_instruction_t *
  duplicateCodeletInstruction(instruction_state_pair *original);

  void doReschedulingOfReadyCodelets(instruction_state_pair *original);

  void createDuplicatedCodelet(instruction_state_pair *inst, int copies = 1);

  bool compareTwoCodelets(instruction_state_pair *original,
                          instruction_state_pair *duplicate);

  void overrideOriginalOperands(instruction_state_pair *original,
                                instruction_state_pair *duplicate);

public:
  dupl_controller_module() = delete;
  dupl_controller_module(DUPL_MODES m, fetch_decode_module *fd,
                         instructions_buffer_module *instBuff);
  dupl_controller_module(fetch_decode_module *fd,
                         instructions_buffer_module *instBuff);

  void duplicateCodelet(instruction_state_pair *inst);

  bool compareCodelets(instruction_state_pair *inst);

  void cleanupDuplication(instruction_state_pair *inst);

  ~dupl_controller_module();
};

} // namespace scm
#endif // __DUPL_CONTROLLER__