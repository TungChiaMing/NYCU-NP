#include "shell/pipe.hpp"
#include "shell/cmdHandler.hpp"

#include <iostream>
#include <sys/wait.h> // signal

bool running = true;

void exitHandler(int) { 
  running = false;
}

int main() {
  setenv("PATH", "bin:.", 1);

  struct sigaction act {};
  act.sa_handler = SIG_IGN;
  act.sa_handler = exitHandler;
  sigaction(SIGINT, &act, nullptr);

  CmdHandler cmdHandler;
  std::cout << "% ";



  while(running) { 
    cmdHandler.runShell(STDIN_FILENO);
  }
  return 0;
}
