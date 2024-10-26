#ifndef CGI_UTIL_HPP
#define CGI_UTIL_HPP

#include <regex>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

#define QUERY_STRING R"(h(\d)=([^&]*)&p\1=([^&]*)&f\1=([^&]*))"

#define NPSHELL_WEBCONSOLE "webconsole/console.html"
#define HTML_CONTENT_TYPE "Content-type: text/html\r\n\r\n"

constexpr int PANEL_N_SERVERS = 5;
constexpr int PANEL_N_NP_HOST = 12;
constexpr std::string_view HTTP200 = "HTTP/1.1 200 OK\r\n";
const std::string PANEL_FORM_METHOD = "GET";
const std::string PANEL_FORM_ACTION = "console.cgi";
const std::string PANEL_TEST_CASE_DIR = "test_case";
const std::string PANEL_DOMAIN = "cs.nycu.edu.tw";

// Informations used for console.cgi application
struct ServerInfo {
  std::string host;
  std::string port;
  std::string fileName;
};

struct ClientInfo {
  int sessionId;
  std::string userCommand;
  std::string shellOutput;
  std::string output_shell();
  std::string output_command();
};
std::string cgi_output_shell(std::string content, int sessionId);
std::string cgi_output_command(std::string content, int sessionId);
std::string cgi_html_escape(std::string context);

std::vector<ServerInfo> parseHTTPQueryString(const char *query);

std::vector<ServerInfo> parseQueryString(const std::string& query);

std::string generateHTTPNPShellWebConsole(const std::vector<ServerInfo>& hosts);

void printEscape(std::ostringstream& out, const std::string& content);


std::string generatePanelTestCaseMenu(const std::string& dir);
std::string generatePanelHostMenu(int num_hosts, const std::string& domain);
std::string generatePanel();
constexpr std::string_view npshell_webconsole = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>NP Project 3 Sample Console</title>
    <link
      rel="stylesheet"
      href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
      integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
      crossorigin="anonymous"
    />
    <link
      href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
      rel="stylesheet"
    />
    <link
      rel="icon"
      type="image/png"
      href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
    />
    <style>
      * {
        font-family: 'Source Code Pro', monospace;
        font-size: 1rem !important;
      }
      body {
        background-color: #212529;
      }
      pre {
        color: #cccccc;
      }
      b {
        color: #01b468;
      }
    </style>
  </head>
  <body>
    <table class="table table-dark table-bordered">
      <thead>
        <tr>
          <th scope="col"></th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td><pre id="s0" class="mb-0"></pre></td>
        </tr>
      </tbody>
    </table>
  </body>
</html>)";

#endif // CGI_UTIL_HPP