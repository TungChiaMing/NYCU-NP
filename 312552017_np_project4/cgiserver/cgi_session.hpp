#ifndef CGI_SESSION_HPP
#define CGI_SESSION_HPP

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <queue>

namespace {
  namespace http = boost::beast::http;
  namespace asio = boost::asio;
  using tcp = boost::asio::ip::tcp;
}

namespace CGI {
  class Session : public std::enable_shared_from_this<Session> {
   friend class Application;
   public:
    Session(asio::io_context* io, tcp::socket&& sock) 
      : io_context(*io), tcpStream(std::move(sock)) { };
  
    void run();
    void handleHttpRequest();
    void handleHttpError();
#ifdef __linux__
    void saveParms(std::vector<std::string> &parms, const std::string &query);
    void addEnv(std::vector<std::string> &envs, const std::string& format, auto... args);
    void forkCGI(const std::string &programPath, const std::vector<std::string>& envp);
#endif
    void doWrite();
    void write(const std::string& data);

    void routeAPI(const std::string &programPath, const std::string &query);
   private:
    asio::io_context& io_context;
    boost::beast::tcp_stream tcpStream;
    boost::beast::flat_buffer buffer;
    http::request<http::string_body> httpReq;
    std::queue<std::string> htmlContent;
  };

}
#endif // CGI_SESSION_HPP