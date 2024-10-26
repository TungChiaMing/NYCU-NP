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


  // server
  auto&& Server = chatServer::getInstance();
  int serverPort = Server.getPortFromArgs(argc, argv);
  int serverSock = Server.createListenSocket(serverPort);
  Server.showServingNetworkInfo(serverPort);
	for(int i = 1 ; i < MAX_CONN ; i++) {
		Server.setServingUserEnv(i, "PATH", "bin:.");
	}

  // all sockets monitoring
	fd_set masterSocks;	
	FD_ZERO(&masterSocks);
	FD_SET(serverSock, &masterSocks);

	// sockets are ready for I/O
	fd_set readySocks;
	FD_ZERO(&readySocks);

	int maxSocks = serverSock; // the max range for select
	while(running){
		// select would modifies readySocks that is really ready
		// so we need to copy original masterSocks to readySocks
		readySocks = masterSocks;

		// maxSocks+1 to contain fd=0
		int ret = select(maxSocks+1, &readySocks, nullptr, nullptr, nullptr);
		if(ret <= 0) continue;


		for ( int fd = 0 ; fd <= maxSocks ; fd++) {
			// iterate selected sockets instead of using map id to prevent user removement 
			if (FD_ISSET(fd, &readySocks)) {
				if(fd == serverSock) {
					// comes new connection
					sockaddr_in clientAddr{};
					socklen_t clientAddrSize = sizeof(clientAddr);
					int clientFd = accept(serverSock, (sockaddr*)&clientAddr, &clientAddrSize);
					FD_SET(clientFd, &masterSocks);	
					if ( clientFd > maxSocks ) maxSocks = clientFd; // clientFd++

					Server.addUser(inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
					Server.sendRawMessage(clientFd, WELCOME_MSG);
					Server.broadcastMessage(LOGIN_MSG(Server.getServingUserName(clientFd), inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port)));
					Server.sendRawMessage(clientFd, "% ");
				} else {
					// handle old connection
					if(!Server.runShellLoop(fd)){
						Server.removeUser(fd);
						FD_CLR(fd, &masterSocks);
					}
				}
			}

		}
	}
	close(serverSock);
	return 0;
}