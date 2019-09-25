#include "instruction_mem.hpp"

using namespace std;

scm::inst_mem_module::inst_mem_module(string const filename ) {
  SCMULATE_INFOMSG(3, "CREATING INSTRUCTION MEOMRY");
  this->is_valid = true;
  // Open the file, if specified, otherwise read from stdio
  fstream file_stream;
  string line;
  if (filename.length() != 0) {
    SCMULATE_INFOMSG(2, "LOADING FILE %s", filename.c_str());
    file_stream.open(filename);
    // Redirect cin
    if (file_stream.is_open()) {
      while (file_stream >> line)
        this->memory.push_back(line);
      file_stream.close();
    } else {
      this->is_valid = false;
    }
  } else {
    while ((cin >> line) && line != "-")
      this->memory.push_back(line);
  }
}

void
scm::inst_mem_module::dumpMemory() {

  auto it = this->memory.rbegin();
  for (; it != this->memory.rend(); it++) 
    cout << "-" << static_cast<int>(it - memory.rbegin()) << " " << *it << endl;
}
