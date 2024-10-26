#include "cgiserver/cgi_application.hpp"
#include "cgiserver/cgi_util.hpp"


int main() {
  std::setbuf(stdout, nullptr);
  asio::io_context io_context;

  // std::cout << HTML_CONTENT_TYPE;

  const char* queryString = getenv("QUERY_STRING");
  auto httpReq = parseHTTPQueryString(queryString);
  std::string npShellWebConsole = generateHTTPNPShellWebConsole(httpReq);
  std::cout << npShellWebConsole;
  
  for (std::size_t sessionId = 0; sessionId < httpReq.size(); sessionId++) {
    std::make_shared<CGI::Application>(io_context, httpReq[sessionId])->run(sessionId);
  }
  io_context.run();  
  
  return 0;
}
