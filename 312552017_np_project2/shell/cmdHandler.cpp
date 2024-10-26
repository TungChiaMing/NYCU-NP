#include "cmdHandler.hpp"
#include "debugger.hpp"
#include "server/chatServer.hpp"

#include <iostream>
#include <sstream> // istringstream
#include <unistd.h> // fork
#include <sys/wait.h> // waitpid




// Run the npshell
void CmdHandler::runShell(int fd) {
  _fd = fd;
  auto&& Server = chatServer::getInstance();
  int id = Server.getServingUserID(_fd);

  std::string line;


  std::getline(std::cin, line); 
  std::queue<NpCommandLine> spiltedNumberedCommands;


  spiltedNumberedCommands = parseCommand(line);


  
  while(!spiltedNumberedCommands.empty()) {
    eol.create();
    
    numberPipeList.next();
    currPipe = numberPipeList.front();
    // auto& prevNumPipe = NumberedPipeCommand::numberPipeList.front();
    NumberedPipeCommand::numberPipeList = &numberPipeList;

 

    auto npCommandLine = std::move(spiltedNumberedCommands.front());

    int readUid = npCommandLine._readUid;
    int writeUid = npCommandLine._writeUid; 
    if(readUid > 0) Server.readUserPipe(id, readUid, currPipe, line, writeUid);
    // else {
    //   auto& prevNumPipe = NumberedPipeCommand::numberPipeList->front();
    //   if(prevNumPipe[0] != -1) {
    //     currPipe.closePipe();
    //     currPipe.set(prevNumPipe[0], -1);
    //     prevNumPipe.closePipe();
    //   }
    //   // numberPipeList.next();
    //   // currPipe = numberPipeList.front();
    // }

    auto&& npCommands = npCommandLine.get();
    for (std::size_t i = 0; i < npCommands.size(); ++i) {

      if (npCommands[i]->getType() != cmdType::EXE) continue;
        
      prevPipe.closePipe();
      prevPipe = currPipe;
      
      bool pipeErr = false;
      

      if (i != npCommands.size() - 1) {

        auto& nextCommand = dynamic_cast<PipeCommand&>(*npCommands[i + 1]); 

        // prevent directly writeUserPipe, e.g., cat test.html | cat >3
        auto userPipeWrite = dynamic_cast<UserPipeCommand*>(npCommands[i + 1].get()); 
        
        if(userPipeWrite != nullptr) {
          if(writeUid > 0 ) Server.writeUserPipe(id, writeUid, currPipe, line, readUid);
        } else {
          nextCommand.createPipe(currPipe); 
          pipeErr = nextCommand.pipeError();
        }

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
  if(running && fd == STDIN_FILENO){
    std::cout << "% ";
  }
}

std::queue<NpCommandLine> CmdHandler::parseCommand(const std::string &command) {
  auto&& Server = chatServer::getInstance();
  int id = Server.getServingUserID(_fd);

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
      std::string env;            
      iss >> env;
      
      std::string _var = Server.getServingUserEnv(id, env); 
      if (_var.size()) std::cout << _var << std::endl;
    } else if(singleCommand == "setenv") {
      std::string env;
      std::string var;
      iss >> env >> var;

      Server.setServingUserEnv(id, env, var);
      setenv(env.c_str(), var.c_str(), true);

    } else if(singleCommand == "who") {
      Server.Who(id);
    } else if(singleCommand == "tell") {
      std::string message;
      int uid;
      iss >> uid;
      std::getline(iss >> std::ws, message);
      Server.Tell(id, uid, message);
    } else if(singleCommand == "yell") {
      std::string message;
      std::getline(iss >> std::ws, message);
      Server.Yell(id, message);
    } else if(singleCommand == "name") {
      std::string name;
      // iss >> name;
      std::getline(iss >> std::ws, name);  
      //  name.pop_back();

      Server.Name(id, name);
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
      bool writeErr = false;
      if(singleCommand[0] == '!') writeErr = true;

      npCommandLine.addCommand(std::make_unique<ExeCommand>(argv));
      npCommandLine.addCommand(std::make_unique<NumberedPipeCommand>(number, writeErr));
      ret.emplace(std::move(npCommandLine));
      // note that "removetag test.html | number |1" count as one command
      // removetag test.html |2 ls | number |1 have cause removetag test.html |2 disappers
      // need to be counted as 
      // removetag test.html |2
      // ls | number |1
    } else if(singleCommand[0] == '>') {
      int uid = std::atoi(singleCommand.c_str() + 1);
      npCommandLine._writeUid = uid;

      npCommandLine.addCommand(std::make_unique<ExeCommand>(argv));
      npCommandLine.addCommand(std::make_unique<UserPipeCommand>()); // prevent directly exec, e.g., ls >2
    } else if (singleCommand[0] == '<') {
      int uid = std::atoi(singleCommand.c_str() + 1);
      npCommandLine._readUid = uid;

    }

    else {
      argv.emplace_back(singleCommand); // binary and its options 
    }
    
  }

  if(!argv.empty()) npCommandLine.addCommand(std::make_unique<ExeCommand>(argv));
  
  if(!npCommandLine.npCommands.empty()) ret.emplace(std::move(npCommandLine));

  return ret;

}