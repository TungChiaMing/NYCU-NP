#include "socks4_handler.hpp" 

#include <iostream>
#include <regex>
#include <fstream>


void Socks4Handler::run() {
  auto self = shared_from_this();
  localSocket.async_wait(asio::socket_base::wait_read, [self](auto ec) {
    if (!ec) {
      self->handleRead();
    }
  });
}

void Socks4Handler::handleRead() {
  if (remoteSendBuffer.empty()) return;

  localSocket.read_some(asio::buffer(remoteSendBuffer));
  auto hdr = *reinterpret_cast<Socks4Header*>(remoteSendBuffer.data());

  tcp::endpoint remoteEndpoint = resolveEndpoint(hdr);

  logConnectionDetails(hdr, remoteEndpoint);

  bool command = (hdr.command == SOCKS4_CMD_CONNECT);
  if (isDestinationIPAddressAllowed(remoteEndpoint.address().to_string(), command)) {
    handleCommand(hdr.command, remoteEndpoint);

  } else {
    std::cout << "<Reply>: Reject" << std::endl;
    Socks4Header reject_hdr = {0, SOCKS4_REPLY_REJECTED, 0, 0};
    localSocket.write_some(asio::buffer(&reject_hdr, sizeof(Socks4Header)));
    localSocket.shutdown(asio::socket_base::shutdown_both);
  }

  
}

tcp::endpoint Socks4Handler::resolveEndpoint(const Socks4Header& hdr) {
  asio::ip::address_v4 addr(ntohl(hdr.dstaddr));
  tcp::endpoint remoteEndpoint;

  if (std::regex_match(addr.to_string(), socks4a)) {
    // allow the use of SOCKS on hosts which are not capable of resolving all domain names.
    // the dstIP will be 0.0.0.x if client cannot resolve the destination host's domain name.
    // Following the NULL byte terminating USERID, 
    // the client must sends the destination domain name and termiantes it with another NULL byte
    std::string userid(remoteSendBuffer.data() + sizeof(Socks4Header));
    std::string domainName(remoteSendBuffer.data() + sizeof(Socks4Header) + userid.length() + 1);
    remoteEndpoint = *resolver.resolve(tcp::v4(), domainName, std::to_string(ntohs(hdr.dstport)));
  } else {
    remoteEndpoint.address(addr);
    remoteEndpoint.port(ntohs(hdr.dstport));
  }

  return remoteEndpoint;
}

void Socks4Handler::logConnectionDetails(const Socks4Header& hdr, const tcp::endpoint& remoteEndpoint) {
  std::cout << "<S_IP>: " << localSocket.remote_endpoint().address() << std::endl;
  std::cout << "<S_PORT>: " << localSocket.remote_endpoint().port() << std::endl;
  std::cout << "<D_IP>: " << remoteEndpoint.address().to_string() << std::endl;
  std::cout << "<D_PORT>: " << remoteEndpoint.port() << std::endl;
  std::cout << "<Command>: " << ((hdr.command == SOCKS4_CMD_CONNECT) ? "CONNECT" : "BIND") << std::endl;
}

void Socks4Handler::handleCommand(uint8_t command, const tcp::endpoint& remoteEndpoint) {
  std::cout << "<Reply>: Accept" << std::endl;
  switch (command) {
  case SOCKS4_CMD_CONNECT: 
    connect(remoteEndpoint); 
    relayTraffic();
    break;
  case SOCKS4_CMD_BIND:
    bind();
    relayTraffic(); 
    break;
  default:
    break;
  }
}

void Socks4Handler::connect(const tcp::endpoint& ep) {
  boost::system::error_code ec;
  remoteSocket.connect(ep, ec);
  if (!ec) {
    reply();
  }
}

void Socks4Handler::bind() {
  bindAcceptor.open(tcp::v4());
  bindAcceptor.bind(tcp::endpoint{tcp::v4(), 0});
  bindAcceptor.listen(1);
  reply(bindAcceptor.local_endpoint().port()); // response back to the client after binding and listening on the specified port
  remoteSocket = bindAcceptor.accept(); // bound endpoint and then establishes the connection
  reply(bindAcceptor.local_endpoint().port()); //  response back to the client indicating the connection was successfully accepted
}


void Socks4Handler::reply(uint16_t port) {
  static Socks4Header hdr{SOCKS4_VERSION, SOCKS4_REPLY_GRANTED, htons(port), 0};
  localSocket.write_some(asio::buffer(&hdr, sizeof(Socks4Header)));
}


void Socks4Handler::asyncReadWrite(boost::asio::ip::tcp::socket& readSocket, 
                                   boost::asio::ip::tcp::socket& writeSocket, 
                                   std::array<char, BUFFER_SIZE>& buffer, 
                                   void (Socks4Handler::*callback)()) {
  auto self = shared_from_this();
  readSocket.async_wait(boost::asio::socket_base::wait_read, [self, &readSocket, &writeSocket, &buffer, callback](auto errCode) {
    if (!errCode) {
      boost::system::error_code ec;
      auto readCount = readSocket.read_some(boost::asio::buffer(buffer), ec);
      if (ec) {
        writeSocket.shutdown(boost::asio::socket_base::shutdown_send);
        return;
      }
      writeSocket.write_some(boost::asio::buffer(buffer, readCount));
      (self.get()->*callback)();
    }
  });
}

void Socks4Handler::readRemote() {
  asyncReadWrite(remoteSocket, localSocket, remoteRecvBuffer, &Socks4Handler::readRemote);
}

void Socks4Handler::writeRemote() {
  asyncReadWrite(localSocket, remoteSocket, remoteSendBuffer, &Socks4Handler::writeRemote);
}

void Socks4Handler::relayTraffic() {
  readRemote();
  writeRemote();
}


bool Socks4Handler::isDestinationIPAddressAllowed(const std::string& ip, bool command) {
  std::ifstream ifs("socks.conf");
  if (!ifs.is_open()) return false;
  std::string permit;
  std::string rule;
  char type;
  while (ifs >> permit >> type >> rule) {
    if ( matchFirewallRule(type, rule, command, ip) ) return true;
  }
  return false;
}