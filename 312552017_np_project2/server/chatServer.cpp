#include "chatServer.hpp"

#include <iostream>
#include <netinet/ip.h> // sockaddr_in
#include <netdb.h> // gethostbyname
#include <unistd.h> // gethostname
#include <arpa/inet.h> // inet_ntop
#include <stdarg.h> // va
#include <signal.h> // SIGUSR1


chatServer::chatServer() {
  signal(SIGUSR1, Kill);
  serverIn = dup(STDIN_FILENO);
  serverOut = dup(STDOUT_FILENO);
  serverErr = dup(STDERR_FILENO);
  _uid = 0;
}

// Meyers Singleton, thread-safe
chatServer& chatServer::getInstance(){
	static chatServer _instance; // only one instance of that local exists in the program
  return _instance;
}

// Show message in other concurrent process
// _uid is created when the process is forked
// utilize _uid to know which shared buffer needed to be showed
void chatServer::Kill(int signo){
  if(signo == SIGUSR1) {
    // note that we can't initialize _instance here
    int id = getInstance()._uid;
    std::cout << getInstance().users[id]->buffer; // signal to different child process
    memset(getInstance().users[id]->buffer, '\0', sizeof(getInstance().users[id]->buffer));
  }
}

// @param uidOrSock: uid is for SHM used, sock is for single proc used
// @param msg: message sent from a client
void chatServer::sendRawMessage(int uidOrSock, const std::string &msg) {
#ifdef CONFIG_SHM
  snprintf(users[uidOrSock]->buffer, sizeof(users[uidOrSock]->buffer), "%s", msg.c_str());
  kill(users[uidOrSock]->pid, SIGUSR1);
#else
  send(uidOrSock, msg.c_str(), msg.length(), 0);   
#endif  
}

void chatServer::broadcastMessage(const std::string &msg) {
  auto broadcastMessageHelper = [this, &msg](UserInfo* user, int id) {
#ifdef CONFIG_SHM
    if (user->isConnected) sendRawMessage(id, msg);
#else
    sendRawMessage(user->sock, msg);
#endif
  };

#ifdef CONFIG_SHM
  for (int i = 1; i < MAX_CONN; i++) broadcastMessageHelper(users[i], i);
#else
  for (auto &[id, user] : users) broadcastMessageHelper(user, id);
#endif
} 

int chatServer::createListenSocket(int port){    
  sockaddr_in addr{}; // memset 0
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

	// open socket fd and setopt
	int listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	constexpr int optval = 1;
	setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	// bind socket to occupy site
  bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  
	// listen socket to start to read
	listen(listenSock, MAX_CONN);

	return listenSock;
}

int chatServer::getPortFromArgs(int argc, char* argv[]) {
	if (argc < 2) {
    throw std::invalid_argument("Usage: " + std::string(argv[0]) + " <port>");
  }
	return std::stoi(argv[1]);
}

void chatServer::showServingNetworkInfo(int port){
  
  // Get the hostname of the machine
  char hostname[1024];
  gethostname(hostname, sizeof(hostname));

  // Get the hostent structure for the hostname
  struct hostent* host = reinterpret_cast<struct hostent*>(gethostbyname(hostname));

  // Convert the first IPv4 address in the hostent structure to an in_addr struct
  struct in_addr ipv4addr = *reinterpret_cast<struct in_addr*>(host->h_addr_list[0]);

  // Convert the in_addr struct to a string
  char ip_address[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &ipv4addr, ip_address, INET_ADDRSTRLEN);

  std::cout << "IP address of " << hostname << " is " << ip_address << ", listening on port " << port << std::endl;
}

void chatServer::createSHMUser() {
  npShmfd = shm_open("/npshm", O_CREAT | O_RDWR, 0666);
  shm_unlink("/npshm");
  if (ftruncate(npShmfd, MAX_CONN * sizeof(struct UserInfo)) == -1) {
  perror("ftruncate");
  }

  info.setData(npShmfd);
  memset(info.getData(), 0, MAX_CONN * sizeof(struct UserInfo));
  for(int i = 1 ; i < MAX_CONN ; ++i) users[i] = static_cast<UserInfo*>(info.getData()) + i;

  // pthread_rwlockattr_t attr;
  // pthread_rwlockattr_init(&attr);
  // pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  // pthread_rwlock_init(info.getLock(), &attr);
  // pthread_rwlockattr_destroy(&attr);	
}

void chatServer::deleteSHMUserFifo(int id) {
  for(int i = 1 ; i < MAX_CONN ; i++ ){
    std::string fileName1 = FIFO_DIR(id, i);
    std::string fileName2 = FIFO_DIR(i, id);
    unlink(fileName1.c_str());
    unlink(fileName2.c_str());
  }
}

void chatServer::addUser(const std::string &ip, int port , int sock, int pid) {
  std::cout << "accept new connection from ip " << ip << ", port " << port << std::endl; 
  int minKey = 1;
#ifdef CONFIG_SHM
  for(int i = 1; i < MAX_CONN; ++i) {
    if (users[i]->isConnected == false ) break;
    minKey++;
  }
#else
  for ( auto& [_id, user] : users) {
    if( minKey < _id) break;
    minKey++;
  }
#endif
  int id = minKey;
  sockID_table[sock] = id;

  std::cout << "add user " << id << std::endl;
#ifndef CONFIG_SHM
  UserInfo *newUser = new UserInfo();
  users[id] = newUser;
#endif



  auto &&user = users[id];
  snprintf(user->ip, sizeof(user->ip), "%s", ip.c_str());
  snprintf(user->name, sizeof(user->name), "(no name)");
  user->port = port;
  user->sock = sock;
  user->isConnected = true;
  user->pid = pid;
  _uid = id;


#ifdef CONFIG_SHM
  for(int i = 1; i < MAX_CONN ; i++) user->userPipes[i].closePipe();
#endif

}

void chatServer::removeUser(int sock) {
  dup2(serverIn, STDIN_FILENO);
  dup2(serverOut, STDOUT_FILENO);
  dup2(serverErr, STDERR_FILENO);

  int id = sockID_table[sock];
  std::string name = std::string(users[id]->name);
  std::cout << name << " close connection from ip " << users[id]->ip << ", port " << users[id]->port << std::endl;
  users[id]->isConnected = false;
  memset(users[id], 0, sizeof(struct UserInfo));
  deleteSHMUserFifo(id);
#ifndef CONFIG_SHM
  delete users[id];
  users.erase(id); 
#endif

  sockID_table.erase(sock);
  std::cout << "remove user " << id << "\n";

  chatServer::broadcastMessage(LOGOUT_MSG(name));
  shutdown(sock, SHUT_RDWR);

  close(sock);

}


// Show information of all users
void chatServer::Who(int id) {
  std::cout << "<ID>\t" << "<nickname>\t" << "<IP:port>\t" << "<indicate me>\n";
  auto WhoHelper = [&](int _id, UserInfo* user) {
      std::cout << _id << "\t" << user->name << "\t" << user->ip << ":" << user->port;
      if (id == _id) std::cout << "\t<-me";
      std::cout << "\n";
  };
#ifdef CONFIG_SHM
  for (int _id = 1; _id < MAX_CONN; _id++) {
    if (users[_id]->isConnected) {
        WhoHelper(_id, users[_id]);
    }
  }
#else
  for (auto &[_id, user] : users) {
    WhoHelper(_id, user);
  }
#endif
}

// Tell user msg with a uid
void chatServer::Tell(int id, int uid, const std::string &msg) {
  bool exist = false;
#ifdef CONFIG_SHM
  exist = users[uid]->isConnected;
#else
  exist = users.find(uid) != users.end();
#endif

  if(!exist) {
    std::cout << "*** Error: user #" << uid << " does not exist yet. ***\n";
    return;
  }
  

  // If the user exists, send the message
  std::string name = users[id]->name;
#ifdef CONFIG_SHM
  sendRawMessage(uid, TELL_MSG(name, msg));
#else
  sendRawMessage(users[uid]->sock, TELL_MSG(name, msg));
#endif
  
}

// Broadcast the message
void chatServer::Yell(int id , const std::string &msg){
  std::string yell_msg = YELL_MSG(users[id]->name, msg);
#ifdef CONFIG_SHM
  for (int uid = 1; uid < MAX_CONN; uid++) {
     if (users[uid]->isConnected) {
        sendRawMessage(uid, yell_msg); 
     }
  }
#else

  for (auto &[uid, user] : users) {
    sendRawMessage(user->sock, yell_msg);
  } 

#endif
}

// Name a user
void chatServer::Name(int id, const std::string &name){
  bool exist = false;
  auto nameHelper = [&](int _id) -> bool {
    if (users[_id]->isConnected && users[_id]->name == name) {
      exist = true;
      return true;
    }
    return false;
  };


#ifdef CONFIG_SHM
  for (int _id = 1; _id < MAX_CONN; ++_id) if(nameHelper(_id)) break;
#else
  for (const auto& [_id, user] : users) if(nameHelper(_id)) break; 
#endif
  if (exist) {
      std::cout << "*** User '" << name << "' already exists. ***\n";
      return;
  }
 
  strcpy(users[id]->name, name.c_str());

  chatServer::broadcastMessage(NAME_MSG(users[id]->ip, users[id]->port, name));
}

void chatServer::writeUserPipe(int id, int writeUid, Pipe &currPipe, const std::string &originalCommand, int readUid) {
  bool userExist = true;
  bool pipeExist = false;
  handleUserPipe(id, writeUid, O_WRONLY, userExist, pipeExist);
  if(!userExist) {
    currPipe.set(-1, handleUserPipeError(id, writeUid, O_WRONLY, UserPipeErr::USER_NOT_EXIST));
    return;
  }

  if(pipeExist == false) {
    std::string sender = users[id]->name;
    std::string receiver = users[writeUid]->name;
    std::string msg;

    bool broadcastWritePipe = true;

#ifdef CONFIG_SHM
    if(readUid != 0) broadcastWritePipe = false; 
#else
    users[writeUid]->userPipes[id].create();
#endif
    // when using SHM, we've already broadcast write pipe msg if read and write at meanwhile
    if(broadcastWritePipe) {
        // if(originalCommand.back() == '\n') originalCommand.pop_back();
        msg +=  "*** " + sender + " (#" + std::to_string(id) + ") just piped '"  + originalCommand + "' to ";
        msg +=  receiver + " (#" + std::to_string(writeUid) + ") ***\n";
        chatServer::getInstance().broadcastMessage(msg);
    }

    currPipe.set(-1, dup(users[writeUid]->userPipes[id][1]));
    
  } else {
    currPipe.set(-1, handleUserPipeError(id, writeUid, O_WRONLY, UserPipeErr::PIPE_ERR));
  }

}


void chatServer::readUserPipe(int id, int readUid, Pipe &currPipe, const std::string &originalCommand, int writeUid) {    
  bool userExist = true;
  bool pipeExist = true;
  handleUserPipe(id, readUid, O_RDONLY, userExist, pipeExist);
  if(!userExist) {
    currPipe.set(handleUserPipeError(id, readUid, O_RDONLY, UserPipeErr::USER_NOT_EXIST), -1);
    return;
  }
  
  if(pipeExist) {
    std::string sender = users[readUid]->name;
    std::string receiver = users[id]->name;
    std::string msg;

    // if(originalCommand.back() == '\n') originalCommand.pop_back();
    msg += "*** " + receiver + " (#" + std::to_string(id) + ") just received from ";
    msg += sender + " (#" + std::to_string(readUid) + ") by '" + originalCommand + "' ***\n";
#ifdef CONFIG_SHM
    if(writeUid) {
      sender = users[writeUid]->name;
      msg +=  "*** " + receiver + " (#" + std::to_string(id) + ") just piped '"  + originalCommand + "' to ";
      msg +=  sender + " (#" + std::to_string(writeUid) + ") ***\n";
    }
    readFIFO_writePipe(users[id]->userPipes[readUid][0]);
    auto fileName = FIFO_DIR(readUid, id);
    unlink( fileName.c_str() );
#endif
    chatServer::getInstance().broadcastMessage(msg);
    currPipe.set(dup(users[id]->userPipes[readUid][0]), -1);
    
#ifndef CONFIG_SHM
    users[id]->userPipes[readUid].closePipe();
#endif
  } else {
    currPipe.set(handleUserPipeError(id, readUid, O_RDONLY, UserPipeErr::PIPE_ERR), -1);
  }

}

// Write to pipe before waiting for client exit
// since we're using NON-BLOCKING Read
// Otherwise, causing cat: -: Resource temporarily unavailable
void chatServer::readFIFO_writePipe(int fd) {
  int temp[2];
  if(pipe(temp) < 0) { }

  char buffer[USER_MSG_LEN];  
  while(1) { // avoid read may failed
    memset(buffer, '\0', sizeof(buffer));  

    int read_len = read(fd, buffer, sizeof(buffer)); 

    if(read_len == -1)  {
      break;
    } else {
      if(write(temp[1], buffer, strlen(buffer)) < 0 ) { }
    }
  }

  close(temp[1]);
  dup2(temp[0], fd);
  close(temp[0]);
}

int chatServer::handleUserPipeError(int id, int uid, int mode, short problemDetails) {
  int dev_null = -1;
  switch (problemDetails)
  {
  case UserPipeErr::USER_NOT_EXIST:
    std::cout << "*** Error: user #" << uid << " does not exist yet. ***\n";
    dev_null = open("/dev/null",mode);  
    break;
  case UserPipeErr::PIPE_ERR:
    if(mode == O_RDONLY) std::cout << "*** Error: the pipe #" << uid << "->#" << id << " does not exist yet. ***\n";
    else std::cout << "*** Error: the pipe #" << id << "->#" << uid << " already exists. ***\n";
    dev_null = open("/dev/null",mode);
    break;
  default:
    break;
  }
  return dev_null;
}

// If using SHM, Using the FIFO
// else just setting the Exist flag
// @param mode: either Read or Write
void chatServer::handleUserPipe(int id, int uid, int mode, bool &userExist, bool &pipeExist) {
  std::string fileName;
  switch (mode)
  {
  case O_WRONLY:
    fileName = FIFO_DIR(id, uid);
#ifdef CONFIG_SHM
    if (uid > MAX_CONN || !users[uid]->isConnected) {
      userExist = false;
      break;
    }
    if(users[uid]->userPipes[id].openFIFO(fileName.c_str(), FileCommand::WRITE) == false) pipeExist = true;
#else
    if (uid > MAX_CONN || users.find(uid) == users.end()) {
      userExist = false;
      break;
    } 
    if(users[uid]->userPipes[id].empty() == false) pipeExist = true;
#endif
    break;
  case O_RDONLY:
    fileName = FIFO_DIR(uid, id);
#ifdef CONFIG_SHM
    if (uid > MAX_CONN || !users[uid]->isConnected) {
      userExist = false;
      break;
    } 
    if(users[id]->userPipes[uid].openFIFO(fileName.c_str(), FileCommand::READ) == false) pipeExist = false;
#else
    if (uid > MAX_CONN || users.find(uid) == users.end()) {
      userExist = false;
      break;
    } 
    if(users[id]->userPipes[uid].empty()) pipeExist = false;
#endif
    break;
  default:
    break;
  }
  
}

void chatServer::sendMessage(int fd, const char* fmt, ...) {
  va_list argList1, argList2;
  va_start(argList1, fmt);
  va_copy(argList2, argList1);
  size_t needed = vsnprintf(nullptr, 0, fmt, argList1) + 1;
  va_end(argList1);
  std::vector<char> buffer(needed);
  vsnprintf(buffer.data(), needed,  fmt, argList2);
  va_end(argList2);
  send(fd, fmt, needed - 1, 0);   
}

int chatServer::getServingUserID(int fd) {
  return sockID_table[fd];
}

const char* chatServer::getServingUserName(int fd) {
  return users[sockID_table[fd]]->name;
}

std::string chatServer::getServingUserEnv(int id, const std::string &env) {
  return env_table[id][env];
}
void chatServer::setServingUserEnv(int id, const std::string &env, const std::string &value) {
  env_table[id][env] = value;
}

bool chatServer::runShellLoop(int fd) {
  int id = sockID_table[fd];

  dup2(fd, STDIN_FILENO);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  // setenv("PATH", "bin:.", 1);
  setenv("PATH", env_table[id]["PATH"].c_str(), true);


  users[id]->runShell(fd);
  

  dup2(serverIn, STDIN_FILENO);
  dup2(serverOut, STDOUT_FILENO);
  dup2(serverErr, STDERR_FILENO);

  if(users[id]->running){
    sendRawMessage(fd, "% ");
    return true;
  }
  else return false;
}