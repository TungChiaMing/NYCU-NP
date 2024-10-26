#ifndef CMDHANDLER_HPP
#define CMDHANDLER_HPP
#include "cmdType.hpp"

#include <queue>
#include <memory>
#include <string>
#include <filesystem>
#include <iostream>
#include <pwd.h>
#include <fstream>
#include <unistd.h>
#include <fcntl.h> // open

class CmdHandler {
 public:
  CmdHandler()
   : status(0), pipelineProcessor{} { pipelineProcessor.fill(0); }
  ~CmdHandler() { }
  
  static npshell::circular_buffer<Pipe>& getNumberPipeList() { return s_numberPipeList; }
  std::queue<NpCommandLine> parseCommand(const std::string &command);
  void runShell(const int fd);
  int status;  
 private:
  static npshell::circular_buffer<Pipe> s_numberPipeList;
  Pipe currPipe; // used to write the pipe (pipe may already have data in it, in this case, like append data to the pipe)
  Pipe prevPipe; // used to read the pipe
  Pipe eol; // used to read EOF
  std::array<pid_t, 2> pipelineProcessor;
};

#endif // CMDHANDLER_HPP