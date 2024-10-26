#include "cgi_application.hpp"
#include "socks4server/socks4_util.hpp"

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

void CGI::Application::connectProxy(int sessionId, const boost::asio::ip::tcp::endpoint& proxy) {
  clientInfo.sessionId = sessionId;
  {
    // Send socks4_request
    char socks4Request[13] = {4, 1, 0, 0, 0, 0, 0, 0, 'M', 'E', 'O', 'W', 0};
    auto&& hdr = *reinterpret_cast<Socks4Header*>(socks4Request);
    tcp::endpoint ep = *resolver.resolve(tcp::v4(), serverinfo.host, serverinfo.port);
    hdr.dstport = htons(ep.port());
    hdr.dstaddr = htonl(ep.address().to_v4().to_ulong());
    tcpSocket.connect(proxy);
    tcpSocket.write_some(asio::buffer(socks4Request, sizeof(socks4Request)));  
  }

  {
    // Receive socks4_reply
    std::array<char, 1024> socks4Reply;
    tcpSocket.read_some(asio::buffer(socks4Reply, sizeof(Socks4Header)));  
    auto hdr = *reinterpret_cast<Socks4Header*>(socks4Reply.data());
    if (hdr.command == SOCKS4_REPLY_GRANTED) forwardforkedNPShellHTTPResp();
  }
}