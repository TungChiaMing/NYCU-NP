#include "socks4server/socks4_handler.hpp"
 
#include <iostream>
#include <stdexcept>
 

class Socks4Server {
 public:
  Socks4Server(boost::asio::io_context* ioc, short unsigned int port) : io_context(*ioc), acceptor(*ioc) {
    acceptor.open(tcp::v4());
    acceptor.set_option(asio::socket_base::reuse_address(true));
    acceptor.bind(tcp::endpoint{tcp::v4(), port});
    acceptor.listen();
    run();
  }

  void run() {
    acceptor.async_accept(io_context, [this](auto ec, tcp::socket socket) {
      if (!ec) {
        io_context.notify_fork(asio::execution_context::fork_prepare);
        auto pid = fork();

        if (pid == -1) {
          exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
          io_context.notify_fork(asio::execution_context::fork_child);
          std::make_shared<Socks4Handler>(&io_context, std::move(socket))->run();
        }
        else {  
          io_context.notify_fork(asio::execution_context::fork_parent);
          run();
        }
      }
    });
  }

 private:
  boost::asio::io_context& io_context;
  tcp::acceptor acceptor;
};

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return EXIT_FAILURE;
  }
  try {
    boost::asio::io_context context;
    boost::asio::signal_set sigs(context, SIGINT);

    signal(SIGCHLD, SIG_IGN);
    sigs.async_wait([&context](auto, auto) { context.stop(); });
    Socks4Server s4(&context, std::atoi(argv[1]));
    context.run();
  } catch (const std::exception& e) {
  // terminate called after throwing an instance of 'boost::wrapexcept<boost::system::system_error>'
  // what():  write_some: Broken pipe
  }


  return 0;
}
