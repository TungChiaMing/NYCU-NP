#ifndef CHATSERVER_HPP
#define CHATSERVER_HPP

#define IP_LEN 16
#define MAX_USER_NAME_LEN 20
#define USER_MSG_LEN 1024
#define MAX_CONN 31
#define WELCOME_MSG "****************************************\n"\
                     "** Welcome to the information server. **\n"\
                     "****************************************\n"
#define LOGIN_MSG(name, _ip, port) ("*** User '" + std::string(name) + "' entered from " + std::string(_ip) + ":" + std::to_string(port) +  ". ***\n")
#define LOGOUT_MSG(name) ("*** User '" + std::string(name) + "' left. ***\n")
#define TELL_MSG(name, msg) ("*** " + std::string(name) + " told you ***: " + msg + "\n" )
#define YELL_MSG(name, msg) ("*** " + std::string(name) + " yelled ***: " + msg + "\n" )
#define NAME_MSG(_ip, port, name) ("*** User from " + std::string(_ip) + ":" + std::to_string(port) + " is named '" + name + "'. ***\n")

#define FIFO_DIR(sender, receiver) ( "./user_pipe/" + std::to_string(sender) + "-" + std::to_string(receiver) )

#include "shell/cmdHandler.hpp"

#include <map>
#include <fcntl.h> // pid_t

struct UserInfo : public CmdHandler {
	int port = -1;
	int sock = -1;

  int pid = -1;
//   pthread_rwlock_t lock;
	char buffer[USER_MSG_LEN] = {};

	char ip[IP_LEN] = {};
	char name[MAX_USER_NAME_LEN] = {};
	bool isConnected = false;
};

class chatServer {
 public:
	static chatServer& getInstance();
	static void Kill(int signo);
	void sendRawMessage(int uidOrSock, const std::string &msg);
	void broadcastMessage(const std::string &msg);

	int createListenSocket(int port);
	int getPortFromArgs(int argc, char* argv[]);
	void showServingNetworkInfo(int port);
	void addUser(const std::string &ip, int port , int sock, int pid=-1);
	void removeUser(int sock);

	bool runShellLoop(int fd);

  void Who(int id);
  void Tell(int id, int uid, const std::string &msg);
  void Yell(int id, const std::string &msg);
  void Name(int id, const std::string &name);

	void readUserPipe(int id, int readUid, Pipe &currPipe, const std::string &originalCommand, int writeUid);
	void writeUserPipe(int id, int writeUid, Pipe &currPipe, const std::string &originalCommand, int readUid);
	void readFIFO_writePipe(int fd);
	
	void createSHMUser();
	void deleteSHMUserFifo(int id);

	void sendMessage(int fd, const char* fmt, ...);
	int getServingUserID(int fd);
	const char* getServingUserName(int fd);
	std::string getServingUserEnv(int id, const std::string &env);
	void setServingUserEnv(int id, const std::string &env, const std::string &value); 
	
 private:
	constexpr static int shmSize = MAX_CONN * sizeof(struct UserInfo);

	chatServer();
  ~chatServer() { };
	int handleUserPipeError(int id, int uid, int mode, short problemDetails);
	void handleUserPipe(int id, int uid, int mode, bool &userExist, bool &pipeExist);
 
	std::map<int, UserInfo*> users;
	std::map<int, std::map<std::string , std::string>> env_table; // <env, string>
	std::map<int, int> sockID_table; // <sock, id>
	npshell::Memmap<UserInfo, shmSize> info;

	int npShmfd;
	int _uid;
	int _serverSock;
	int serverIn;
	int serverOut;
	int serverErr;
	enum UserPipeErr {
		USER_NOT_EXIST= 1,
		PIPE_ERR,
	};
};

#endif // CHATSERVER_HPP