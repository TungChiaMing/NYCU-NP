#ifndef CMDHANDLER_HPP
#define CMDHANDLER_HPP
#define MAX_CONN 31

#include "cmdType.hpp"
#include "util.hpp"

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
   : status(0), running(true), pipelineProcessor{} { pipelineProcessor.fill(0); }
  ~CmdHandler() { }
  
  std::queue<NpCommandLine> parseCommand(const std::string &command);
  void runShell(int fd);

 
  npshell::circular_buffer<Pipe> numberPipeList;
  std::array<Pipe, MAX_CONN> userPipes;
  int status;  
  bool running;
 private:
  std::array<pid_t, 2> pipelineProcessor;
  Pipe currPipe; // used to write the pipe (pipe may already have data in it, in this case, like append data to the pipe)
  Pipe prevPipe; // used to read the pipe
  Pipe eol; // used to read EOF
  int _fd;
};

#endif // CMDHANDLER_HPP