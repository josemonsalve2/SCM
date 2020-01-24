#include "instruction_mem.hpp"

using namespace std;

bool
scm::inst_mem_module::loader(string const filename) {
    SCMULATE_INFOMSG(2, "LOADING FILE %s", filename.c_str());
    fstream file_stream;
    string line;
    file_stream.open(filename);

    if (file_stream.is_open()) {
      while (std::getline(file_stream, line))
        if (line.length() != 0 ) {
          this->memory.push_back(line);
        }
      file_stream.close();
    } else {
      return false;
    }
    return true;
}

scm::inst_mem_module::inst_mem_module(string const filename ) {
  SCMULATE_INFOMSG(3, "CREATING INSTRUCTION MEOMRY");
  this->is_valid = true;
  // Open the file, if specified, otherwise read from stdio
  string line;
  if (filename.length() != 0) {
    this->is_valid = this->loader(filename);
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
