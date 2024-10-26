#include "cgiserver/cgi_util.hpp"
 
#include <memory> 
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
 

namespace { 
  namespace asio = boost::asio;
  using tcp = boost::asio::ip::tcp; 
}
#define BUFFER_SIZE 1024


using namespace std;

boost::asio::io_context io_context;


class NPShellApp : public std::enable_shared_from_this<NPShellApp> {
public:
	NPShellApp(boost::asio::io_context& io, tcp::socket socket, std::shared_ptr<tcp::socket> client_socket, ServerInfo info, int sid)
	: serverSocket(std::move(socket)), clientSocket(client_socket), resolver(io), serverinfo({info.host, info.port, info.fileName}) {
	
		sessionId = sid;
 
		npServerInput.open("test_case/" + serverinfo.fileName);
	 
	}

	void start() {
		auto self = shared_from_this();
		resolver.async_resolve(serverinfo.host, serverinfo.port, [this, self](auto ec, auto endpoint) {
			if (!ec) {
				serverSocket.async_connect(*endpoint, [this, self](auto errCode) {
					if (!errCode) {
						auto host = serverSocket.remote_endpoint().address();
						auto port = serverSocket.remote_endpoint().port();
						std::cout << "Receive connection to the http server: " <<  host << ":" << port ;
						std::cout << " with panel session " << sessionId << std::endl;
						do_read();
					}
				});
 
			}
		});
	}

private:
	void do_read() {
		auto self = shared_from_this();
		serverSocket.async_read_some(boost::asio::buffer(npShellOutput, BUFFER_SIZE), [this, self](auto ec, std::size_t) {
			if (!ec) {
				std::string output = npShellOutput; 
				handleHttpResp(cgi_output_shell(npShellOutput, sessionId));

				memset(npShellOutput, '\0', sizeof(npShellOutput));
				memset(npShellCommand, '\0', sizeof(npShellCommand));

				if (output.find("%") != string::npos) {
					std::string npCommand;
					if(std::getline(npServerInput, npCommand)) {
						npCommand += "\n";
						// std::cout << npCommand; 
						handleHttpResp(cgi_output_command(npCommand, sessionId));
					} 
 
					handleBackendReq(npCommand);
				} else {
					do_read();
				}
			}
		});
	}

	void handleHttpResp(const std::string &ret) {
		auto self = shared_from_this();
		boost::asio::async_write(*clientSocket, boost::asio::buffer(ret.c_str(), ret.size()), [this, self](auto ec, std::size_t) {
			if (ec) {
				std::cout << "handleHttpResp() error\n";
			}
		});
	}

	void handleBackendReq(const std::string &cmd) {
		strcpy(npShellCommand, cmd.c_str());
		auto self = shared_from_this();
		boost::asio::async_write(serverSocket, boost::asio::buffer(npShellCommand, strlen(npShellCommand)), [this, self](auto ec, std::size_t) {
			if (!ec) { 
				std::string curr_cmd{npShellCommand};
				if (curr_cmd.find("exit") != std::string::npos) {
					serverSocket.close();
					std::cout << "panel session " << sessionId << " close" << std::endl;
				} else {
					do_read();
				}
			}
		});
	}
 
	tcp::socket serverSocket;
	ServerInfo serverinfo;
	std::shared_ptr<tcp::socket> clientSocket;
	tcp::resolver resolver;
	std::ifstream npServerInput;
  int sessionId;
	char npShellCommand[BUFFER_SIZE] = {'\0'};
	char npShellOutput[BUFFER_SIZE] = {'\0'};
};



class Session : public std::enable_shared_from_this<Session> {
public:
	Session(tcp::socket socket)
		: client_(std::move(socket)) { };

	void start() { 
    auto self = shared_from_this();
		client_.async_read_some(boost::asio::buffer(httpReq, BUFFER_SIZE), [this, self](auto ec, std::size_t) {
		if (!ec) {
				self->handleHttpRequest();
			}
		});
	}

private:
	template<typename... Args>
	std::string concat(const std::string& format, Args... args) {
		std::ostringstream oss;
		oss << format;
		// Using a fold expression to unpack and insert arguments into the stream
		(oss << ... << args);
		return oss.str();
	}
	void handleHttpRequest() {
		string request = httpReq;
		vector<string> request_header;
		vector<string> httpQuery;

		boost::split(request_header, request, boost::is_any_of("\r\n"), boost::token_compress_on);
		boost::split(httpQuery, request_header[0], boost::is_any_of(" "), boost::token_compress_on);

		std::string queryUrl = httpQuery[1];

		std::vector<std::string> splitter;
		boost::split(splitter, queryUrl, boost::is_any_of("?"));
		if (splitter.size() < 2) splitter.emplace_back(""); // no ?, just FQDN


		std::string httpUrlPath = concat(".", splitter[0]);
		std::string httpUrlQuery = splitter[1];

		std::cout << "path: " << httpUrlPath << "\n" ;
		std::cout << "query: " << httpUrlQuery << "\n" ;

		routeAPI(httpUrlPath, httpUrlQuery);

	}
	void routeAPI(const std::string &programPath, const std::string &query) {
		auto self = shared_from_this();

		std::string httpRsp{HTTP200};
		httpRsp += "\0";
		boost::asio::async_write(client_, boost::asio::buffer(httpRsp, httpRsp.size()),
			[this, self, programPath, query](boost::system::error_code ec, size_t) {
				if (!ec) {
					if (programPath == "./panel.cgi") {
					string panel_page = generatePanel();
					boost::asio::async_write(client_, boost::asio::buffer(panel_page.c_str(), panel_page.length()),
						[this, self](boost::system::error_code errCode, size_t) {
							if (!errCode) {
								cout << "HTTP Response the Server Panel" << endl;
							}
						});
					}
					else if (programPath == "./console.cgi") {
						auto queryNPServer = parseQueryString(query);
						string console_page = generateHTTPNPShellWebConsole(queryNPServer);
						boost::asio::async_write(client_, boost::asio::buffer(console_page.c_str(), console_page.length()),
							[this, self, queryNPServer](boost::system::error_code errCode, size_t) {
								if (!errCode) {
									cout << "HTTP Response the NP shell webconsole" << endl;
									shared_ptr<tcp::socket> shared_client_ = make_shared<tcp::socket>(move(client_));

									for (std::size_t i = 0; i < queryNPServer.size(); i++) {
										if (queryNPServer[i].host != "" && queryNPServer[i].port != "" && queryNPServer[i].fileName != "") {
											std::cout << "Querying the NP server: " << queryNPServer[i].host << ":" << queryNPServer[i].port << " with input file: " << queryNPServer[i].fileName << std::endl;
											tcp::socket remote_(io_context);
											make_shared<NPShellApp>(io_context, move(remote_), shared_client_, queryNPServer[i], i)->start();
										}
									}
								}
							});
					}
					else {
						client_.close();
					}
				}
			});
	}

	tcp::socket client_; 
	char httpReq[BUFFER_SIZE] = {'\0'};

};


class HttpServer {
public:
  HttpServer(boost::asio::io_context& io, int port)
    : acceptor(io) {
      auto endpoint = tcp::endpoint(tcp::v4(), port);
      acceptor.open(tcp::v4());
      acceptor.set_option(asio::socket_base::reuse_address(true));
      acceptor.bind(endpoint);
      acceptor.listen(asio::socket_base::max_connections);
			do_accept();
		}

private:
	void do_accept() {
		acceptor.async_accept([this](auto ec, tcp::socket socket) {
      if (!ec) { 
        make_shared<Session>(move(socket))->start();
      }
      do_accept();
    });
	}

	tcp::acceptor acceptor;
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

	try {
	 
    asio::signal_set sigs(io_context, SIGINT);
    sigs.async_wait(
      [](auto, size_t) { 
        io_context.stop(); 
    });


		HttpServer s(io_context, atoi(argv[1]));
		io_context.run();
	}
	catch (const exception& e) {
		cerr << "Exception: " << e.what() << endl;
	}

	return 0;
}