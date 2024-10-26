#ifndef CGI_APPLICATION_HPP
#define CGI_APPLICATION_HPP
#include "cgi_session.hpp"
#include "cgi_util.hpp"

#include <fstream>

namespace CGI {
  class Application : public std::enable_shared_from_this<Application> {
  public:
    Application(asio::io_context& io, ServerInfo info, std::shared_ptr<CGI::Session> session = nullptr)
      : serverinfo({info.host, info.port, ("test_case/" + info.fileName)}), cgiInput(serverinfo.fileName), cgiSession(session), tcpSocket(io), resolver(io) {};
    void run(int sessionId);
    // void forwardNPShellHTTPResp();
    void forwardforkedNPShellHTTPResp();
    void handleShellInteraction(std::function<void(const std::string&)> outputHandler, std::function<void()> repeatHandler);
  private:
    ClientInfo clientInfo;
    ServerInfo serverinfo;
    std::ifstream cgiInput; // class member init not in order of init list
    std::shared_ptr<CGI::Session> cgiSession;
    tcp::socket tcpSocket;
    tcp::resolver resolver;
  };

}

#endif // CGI_APPLICATION_HPP