#ifndef SOCKS4_HANDLER_HPP
#define SOCKS4_HANDLER_HPP

#include "socks4_util.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

#define BUFFER_SIZE 1024
namespace { 
  namespace asio = boost::asio;
  using tcp = boost::asio::ip::tcp;
}


class Socks4Handler : public std::enable_shared_from_this<Socks4Handler> {
 public:
  Socks4Handler(asio::io_context* io, tcp::socket&& sock) 
    : io_context(*io), localSocket(std::move(sock)), resolver(*io), remoteSocket(*io), bindAcceptor(*io) { } ;

  void run();

 private:
  void handleRead();
  tcp::endpoint resolveEndpoint(const Socks4Header& hdr);
  void logConnectionDetails(const Socks4Header& hdr, const asio::ip::tcp::endpoint& remoteEndpoint);
  void handleCommand(uint8_t command, const asio::ip::tcp::endpoint& remoteEndpoint);

  void connect(const tcp::endpoint& ep);
  void reply(uint16_t port = 0);
  void asyncReadWrite(boost::asio::ip::tcp::socket& readSocket, 
                      boost::asio::ip::tcp::socket& writeSocket, 
                      std::array<char, BUFFER_SIZE>& buffer, 
                      void (Socks4Handler::*callback)());
  void readRemote();
  void writeRemote();
  void relayTraffic();
  void bind();

  bool isDestinationIPAddressAllowed(const std::string& ip, bool command);

  asio::io_context& io_context;
  tcp::socket localSocket;
  tcp::resolver resolver;
  tcp::socket remoteSocket;
  tcp::acceptor bindAcceptor;

  std::array<char, BUFFER_SIZE> remoteSendBuffer;
  std::array<char, BUFFER_SIZE> remoteRecvBuffer;
};


#endif // SOCKS4_HANDLER_HPP