#include "cmdType.hpp"
#include "../debugger.hpp"
#include "cmdHandler.hpp"

#include <unistd.h> // execvp
#include <iostream> // cerr
#include <sstream> // stringstream



void ExeCommand::run() {
  std::vector<char*> argv(m_argv.size() + 1); // memory allocated
  for (size_t i = 0; i < m_argv.size(); ++i) { argv[i] = const_cast<char*>(m_argv[i].c_str());}
  argv[m_argv.size()] = nullptr;
  auto errCode = execvp(argv[0], argv.data());

  if (errCode == -1) {
    std::cerr << "Unknown command: [" << m_argv[0] << "].\n"; 
  }
}

void PipeCommand::createPipe(Pipe& currPipe) { currPipe.create(); }

void FileCommand::createPipe(Pipe& currPipe) {  currPipe.openFile(m_filename.c_str(), FileCommand::WRITE); }

void FileCommand::createFileWithMode(Pipe& currPipe, const std::pair<std::string, fileMode>& file) {
  auto& [fileName, fileMode] = file;
  currPipe.openFile(fileName.c_str(), fileMode);
}

npshell::circular_buffer<Pipe>* NumberedPipeCommand::numberPipeList = nullptr;
void NumberedPipeCommand::createPipe(Pipe& currPipe) { 
  currPipe = numberPipeList->get(m_number); // get already wroten pipe, and prepare to write again
  if (currPipe.empty()) {
    currPipe.create();
    numberPipeList->set(m_number, currPipe);
  }
}
