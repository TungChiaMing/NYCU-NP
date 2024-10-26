#include "cgiserver/cgi_application.hpp"
#include "cgiserver/cgi_util.hpp"


int main() {
  std::setbuf(stdout, nullptr);
  asio::io_context io_context;

  // std::cout << HTML_CONTENT_TYPE;

  const char* queryString = getenv("QUERY_STRING");
  auto httpReq = parseHTTPQueryString(queryString);


  auto socks4Server = httpReq.back();

  httpReq.pop_back();


  std::string npShellWebConsole = generateHTTPNPShellWebConsole(httpReq);
  std::cout << npShellWebConsole;
  
 

  tcp::resolver resolver(io_context);
  tcp::endpoint socks4ServerEP = *resolver.resolve(socks4Server.host, socks4Server.port);


  for (std::size_t sessionId = 0; sessionId < httpReq.size() ; sessionId++) {
    std::make_shared<CGI::Application>(io_context, httpReq[sessionId])->connectProxy(sessionId, socks4ServerEP);
  }
  io_context.run();  
  
  return 0;
}
