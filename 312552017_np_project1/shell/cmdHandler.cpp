#include "cmdHandler.hpp"
#include "debugger.hpp"

#include <iostream>
#include <sstream> // istringstream
#include <unistd.h> // fork
#include <sys/wait.h> // waitpid

extern bool running;

npshell::circular_buffer<Pipe> CmdHandler::s_numberPipeList;

// Run the npshell
void CmdHandler::runShell(const int fd) {
  std::string line;
  if(fd != STDIN_FILENO) dup2(fd, STDIN_FILENO);
  else std::getline(std::cin, line); 
  std::queue<NpCommandLine> spiltedNumberedCommands;


  spiltedNumberedCommands = parseCommand(line);



  while(!spiltedNumberedCommands.empty()) {
    eol.create();
    
    s_numberPipeList.next();
    // auto& prevNumPipe = NumberedPipeCommand::numberPipeList.front();
    currPipe = s_numberPipeList.front();
    

    auto npCommandLine = std::move(spiltedNumberedCommands.front());





    auto&& npCommands = npCommandLine.get();
    for (std::size_t i = 0; i < npCommands.size(); ++i) {

      if (npCommands[i]->getType() != cmdType::EXE) continue;
        
      prevPipe.closePipe();
      prevPipe = currPipe;
      
      bool pipeErr = false;
      

      if (i != npCommands.size() - 1) {
          
        auto& nextCommand = dynamic_cast<PipeCommand&>(*npCommands[i + 1]); 
      
        nextCommand.createPipe(currPipe); 
        pipeErr = nextCommand.pipeError();
      }
      
      auto pid = fork();
      if (pid == 0) { 

        prevPipe.read();
        currPipe.write(pipeErr);

        npCommands[i]->run();

        exit(EXIT_SUCCESS);
      }  
      if (pipelineProcessor[0]) waitpid(pipelineProcessor[0], nullptr, 0);
      pipelineProcessor[0] = pipelineProcessor[1];
      pipelineProcessor[1] = pid; 
    }

    prevPipe.closePipe(); 
    
    const auto& lastCommand = npCommands.back();

    if ( lastCommand->getType() != cmdType::NUMBERED_PIPE) {
      waitpid(pipelineProcessor[1], &status, 0);
      currPipe.closePipe();
    } else {
      eol.readEOF(); // handling numbered pipe output race condition
    }

    npCommands.clear(); 
    spiltedNumberedCommands.pop();
  }
  if(running && fd==STDIN_FILENO) {
    std::cout << "% ";

  }
}

std::queue<NpCommandLine> CmdHandler::parseCommand(const std::string &command) {
  std::queue<NpCommandLine> ret;
  NpCommandLine npCommandLine;

  std::vector<std::string> argv;
  std::istringstream iss(command);
  std::string singleCommand;
  while(iss >> singleCommand) {
    if(singleCommand == "exit") {
      running = false;
      eol.closePipe();
      prevPipe.closePipe();
      currPipe.closePipe();

    } else if(singleCommand == "printenv") {
      std::string var;            
      iss >> var;
      
      auto value = std::getenv(var.c_str());
      if (value) std::cout << value << std::endl;
    } else if(singleCommand == "setenv") {
      std::string var;
      std::string value;
      iss >> var >> value;
      
      setenv(var.c_str(), value.c_str(), true);
      
    } 
    
    else if(singleCommand == "|") {
      npCommandLine.addCommand(std::make_unique<ExeCommand>(argv));
      npCommandLine.addCommand(std::make_unique<PipeCommand>());
    } else if(singleCommand == ">") {
      std::string fileName;
      iss >> fileName;
      npCommandLine.addCommand(std::make_unique<ExeCommand>(argv));
      npCommandLine.addCommand(std::make_unique<FileCommand>(fileName));
    } else if(singleCommand[0] == '|' or singleCommand[0] == '!') {
      int number = std::atoi(singleCommand.c_str() + 1);


      npCommandLine.addCommand(std::make_unique<ExeCommand>(argv));
      bool writeErr = false;
      if(singleCommand[0] == '!') writeErr = true;
      npCommandLine.addCommand(std::make_unique<NumberedPipeCommand>(number, writeErr));
      // note that "removetag test.html | number |1" count as one command
      // removetag test.html |2 ls | number |1 have cause removetag test.html |2 disappers
      // need to be counted as 
      // removetag test.html |2
      // ls | number |1
      ret.emplace(std::move(npCommandLine));
    } 

    else {
      argv.emplace_back(singleCommand); // binary and its options 
    }
    
  }

  if(!argv.empty()) npCommandLine.addCommand(std::make_unique<ExeCommand>(argv));
  
  if(!npCommandLine.npCommands.empty()) ret.emplace(std::move(npCommandLine));

  return ret;

}