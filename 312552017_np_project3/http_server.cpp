#include "cgiserver/cgi_session.hpp"
#include <iostream>
#include <stdexcept>
#include <boost/asio.hpp>


class HttpServer {
public:
  HttpServer(asio::io_context* io, int port) : io_context(*io), acceptor(*io) {
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    acceptor.open(tcp::v4());
    acceptor.set_option(asio::socket_base::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen(asio::socket_base::max_connections);
  };
  void run() {
    acceptor.async_accept(io_context, [this](auto ec, tcp::socket socket) {
      if (!ec) {
        std::make_shared<CGI::Session>(&io_context, std::move(socket))->run();
      }
      run(); // accept the next connection
    });
  }
private:
  asio::io_context& io_context;
  tcp::acceptor acceptor;
};


int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

  try {
    asio::io_context io_context;
    asio::signal_set sigs(io_context, SIGINT);

    sigs.async_wait(
      [&io_context](auto, size_t) { 
        io_context.stop(); 
    });

    HttpServer httpserver(&io_context, std::atoi(argv[1]));
    httpserver.run();
    io_context.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}