#include "shell/pipe.hpp"
#include "shell/cmdHandler.hpp"
#include "server/chatServer.hpp"
#include "debugger.hpp"

#include <iostream>
#include <sys/wait.h> // signal
#include <arpa/inet.h> // accept

namespace {
  bool running = true;

  void exitHandler(int sig, siginfo_t* sender, void*) { 
    pid_t pid;
    int status;
    int saved_errno;
    switch (sig) {
      case SIGINT:
        running = false;
        break;
      case SIGCHLD:
        // Loop through all terminated child processes
        // return immediately if no child has exited
        saved_errno = errno;
        while ((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0) {} // re-write the errno
        errno = saved_errno; // cleared on subsequent successful calls.
        break;
      default:
        break;
    }
    
  }
}

int main(int argc, char *argv[]) {
  // shell
  setenv("PATH", "bin:.", 1);

  struct sigaction act {};
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;  // use the sa_sigaction field
  act.sa_sigaction = exitHandler;
  sigaction(SIGINT, &act, nullptr);  // Handle SIGINT with exitHandler
  act.sa_flags = SA_SIGINFO | SA_RESTART;  // Use SA_RESTART to automatically restart syscalls when it's interrupted
  sigaction(SIGCHLD, &act, nullptr);  // Handle SIGCHLD to reap zombies


  CmdHandler cmdHandler;


  // server
  auto&& Server = chatServer::getInstance();
  int serverPort = Server.getPortFromArgs(argc, argv);
  int serverSock = Server.createListenSocket(serverPort);
  Server.showServingNetworkInfo(serverPort);
  Server.setServingUserEnv(1, "PATH", "bin:."); // only serve one user

  sockaddr_in clientAddr{};
  while(running) { 
    socklen_t clientAddrSize = sizeof(clientAddr);
    int clientFd = accept(serverSock, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientFd == -1) {
      break;
    }

    pid_t pid = fork();
    if ( pid == 0 ) {
      close(serverSock);
      setenv("PATH", "bin:.", 1);
      // add user
      Server.addUser(inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd, getpid());
      Server.sendRawMessage(clientFd, "% ");
  
      while(Server.runShellLoop(clientFd) && running)
        ;
      Server.removeUser(clientFd);
      close(clientFd);
      exit(EXIT_SUCCESS);
    } else {
      close(clientFd);  
    }
  }
  return 0;
}
