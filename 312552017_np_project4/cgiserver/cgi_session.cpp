#include "cgi_session.hpp"
#include "cgi_util.hpp"
#include <boost/algorithm/string.hpp>

#ifdef __linux__
#include <sys/wait.h>
#endif

namespace {
  template<typename... Args>
  std::string concat(const std::string& format, Args... args) {
    std::ostringstream oss;
    oss << format;
    // Using a fold expression to unpack and insert arguments into the stream
    (oss << ... << args);
    return oss.str();
  }

}


void CGI::Session::run() {
  httpReq.clear();
  auto self = shared_from_this();

  http::async_read(tcpStream, buffer, httpReq, [self](auto ec, std::size_t) {
    if (ec == http::error::end_of_stream) {
      self->handleHttpError();
    }

    if (!ec) {
      self->handleHttpRequest();
    } 
  });
}

void CGI::Session::handleHttpError() { 
  tcpStream.socket().shutdown(tcp::socket::shutdown_send); 
}


void CGI::Session::handleHttpRequest() {
  std::vector<std::string> splitter;
  boost::split(splitter, httpReq.target(), boost::is_any_of("?"));
  if (splitter.size() < 2) splitter.emplace_back(""); // no ?, just FQDN

  std::string path;
  path = concat(".", splitter[0]);
  std::string &query = splitter[1];

  std::vector<std::string> parms;

  const char* queryString = splitter[1].c_str();

  auto queryTest = parseHTTPQueryString(queryString);

  for(auto &[host, port, cgiInputFile] : queryTest) {
    std::cout << "Querying the NP server: ";
    std::cout << host << ":" << port << " with input file: "<< cgiInputFile << std::endl;
  }
  

#ifdef _WIN32
  routeAPI(path, query);
#else
  saveParms(parms, query);
  forkCGI(path, parms);
#endif  
  
}
#ifdef __linux__
void CGI::Session::saveParms(std::vector<std::string> &parms, const std::string &query) {
  auto httpHost = httpReq["Host"].data();
  auto target = httpReq.target().data();
  auto requestMethod = httpReq.method_string().data();
  auto serverAddr = tcpStream.socket().local_endpoint().address().to_string();
  auto remoteAddr = tcpStream.socket().remote_endpoint().address().to_string();
  auto serverPort = tcpStream.socket().local_endpoint().port();
  auto remotePort = tcpStream.socket().remote_endpoint().port();
  addEnv(parms, "REQUEST_METHOD=", requestMethod);
  addEnv(parms, "REQUEST_URI=", target);
  addEnv(parms, "QUERY_STRING=", query);
  addEnv(parms, "SERVER_PROTOCOL=HTTP/", httpReq.version() / 10, '.', httpReq.version() % 10);
  addEnv(parms, "HTTP_HOST=", httpHost);
  addEnv(parms, "SERVER_ADDR=", serverAddr);
  addEnv(parms, "SERVER_PORT=", serverPort);
  addEnv(parms, "REMOTE_ADDR=", remoteAddr);
  addEnv(parms, "REMOTE_PORT=", remotePort);
}

void CGI::Session::addEnv(std::vector<std::string> &envs, const std::string& format, auto... args) {
  envs.emplace_back(concat(format, args...));
  // std::cout << envs.back() << std::endl;
}

 

void CGI::Session::forkCGI(const std::string &programPath, const std::vector<std::string>& envp) {
  io_context.notify_fork(asio::execution_context::fork_prepare);
  pid_t pid = fork();
  if (pid == 0) {
    io_context.notify_fork(asio::execution_context::fork_child);
    int cgiFile = open(programPath.c_str(), O_RDONLY);
    if (cgiFile == -1) {
      // std::cout << "[404] | GET | " << programPath << std::endl;
      io_context.stop();
      return;
    }

    
    auto self = shared_from_this();
    tcpStream.async_write_some(asio::buffer(HTTP200), [self, cgiFile, programPath, envp](auto ec, std::size_t) {
      if (!ec) {
        int fd = self->tcpStream.socket().native_handle();
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

        std::vector<char*> envp_c;
        for (const auto& env : envp) envp_c.push_back(const_cast<char*>(env.c_str()));
        envp_c.push_back(nullptr);
        char* argv[] = { const_cast<char*>(programPath.c_str()), nullptr };

        auto errCode = fexecve(cgiFile, argv, envp_c.data());
        if (errCode == -1) {
            close(cgiFile);
            self->io_context.stop();
            return;
        }
      }
    });
      
  } else {
      io_context.notify_fork(asio::execution_context::fork_parent);
      waitpid(pid, nullptr, 0);
  }
}
#endif

void  CGI::Session::write(const std::string& data) {
  bool isEmpty = htmlContent.empty();
  htmlContent.emplace(data);
  if (isEmpty) doWrite();
}

void CGI::Session::doWrite() {
  if(htmlContent.empty()) return;
  auto self = shared_from_this();
  tcpStream.async_write_some(asio::buffer(htmlContent.front()), [self](auto ec, auto) {
    if (!ec) {
      self->htmlContent.pop();
      self->doWrite();
    }
  });
}


#include "cgi_application.hpp"
void CGI::Session::routeAPI(const std::string &programPath, const std::string &query) {
  auto self = shared_from_this();
  if(programPath == "./panel.cgi") {
    auto res = std::make_shared<http::response<http::string_body>>(http::status::ok, httpReq.version());
    res->set(http::field::server, "NP");
    res->set(http::field::content_type, "text/html");
    res->keep_alive(httpReq.keep_alive());
    res->body() = generatePanel();
    res->prepare_payload();

    http::async_write(tcpStream, *res, [self, res](auto, auto) {
      self->handleHttpError();
    });
  } else if(programPath == "./console.cgi") {

    std::cout << "route to " << programPath << "with query " << query.c_str() << "\n";
    auto queryString = parseHTTPQueryString(query.c_str());

    std::ostringstream oss;
    oss << HTTP200 << HTML_CONTENT_TYPE;
    oss << generateHTTPNPShellWebConsole(queryString) << std::endl;
    auto consoleHTML = oss.str();
    tcpStream.async_write_some(asio::buffer(consoleHTML), [self, queryString](auto ec, std::size_t) {
      if(!ec) {
        for (std::size_t sessionId = 0; sessionId < queryString.size(); sessionId++) {
          std::make_shared<CGI::Application>(self->io_context, queryString[sessionId], self)->run(sessionId);
        }
            
      }
    });
  }
}