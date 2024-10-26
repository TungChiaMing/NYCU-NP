#include"cgi_application.hpp"

#include <boost/regex.hpp>

void CGI::Application::run(int sessionId) {
  clientInfo.sessionId = sessionId;
  auto self = shared_from_this();
  resolver.async_resolve(tcp::v4(), self->serverinfo.host, self->serverinfo.port, [self](auto ec, auto endpoint) {
    if (!ec) {
      self->tcpSocket.async_connect(*endpoint, [self](auto errCode) {
        if (!errCode) {
          self->forwardforkedNPShellHTTPResp(); // write to the frontend http
        }
      });
    }
  });
}

// void CGI::Application::forwardNPShellHTTPResp() {
//     auto outputToSession = [this](const std::string& output) {
//         this->cgiSession->write(output);
//     };
//     auto repeatFunction = [this]() {
//         this->forwardNPShellHTTPResp();
//     };
//     handleShellInteraction(outputToSession, repeatFunction);
// }

void CGI::Application::forwardforkedNPShellHTTPResp() {
  auto outputToStdout = [](const std::string& output) {
      std::cout << output;
  };
  auto repeatFunction = [this]() {
      this->forwardforkedNPShellHTTPResp();
  };
  handleShellInteraction(outputToStdout, repeatFunction);
}

void CGI::Application::handleShellInteraction(std::function<void(const std::string&)> outputHandler, std::function<void()> repeatHandler) {
  static boost::regex delim("(\n)|(% )");
  auto self = shared_from_this();
  self->clientInfo.shellOutput.clear();
  asio::async_read_until(self->tcpSocket, asio::dynamic_buffer(self->clientInfo.shellOutput), delim, [self, outputHandler, repeatHandler](auto ec, auto) {
    if (!ec) {
      outputHandler(self->clientInfo.output_shell());
      if (self->clientInfo.shellOutput.find("% ") == std::string::npos) {
        repeatHandler();
        return;
      }

      self->clientInfo.userCommand.clear();
      if (std::getline(self->cgiInput, self->clientInfo.userCommand)) {
        self->clientInfo.userCommand += '\n';
        outputHandler(self->clientInfo.output_command());
        self->tcpSocket.async_write_some(asio::buffer(self->clientInfo.userCommand), [self, repeatHandler](auto err, auto) {
            if (!err) repeatHandler();
        });
      } else {
        self->tcpSocket.shutdown(tcp::socket::shutdown_send);
      }
    }
  });
}

