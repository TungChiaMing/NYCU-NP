#include "cgi_util.hpp"
#include <cstring>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>


std::vector<ServerInfo> parseQueryString(const std::string& query) {
  std::vector<ServerInfo> ret;
  std::vector<std::string> tmp_query;
  boost::split(tmp_query, query, boost::is_any_of("&"), boost::token_compress_on);

  for (std::size_t i = 0; i < tmp_query.size(); i += 3) {
    ServerInfo serverinfo;
    serverinfo.host = tmp_query[i].substr(tmp_query[i].find("=") + 1);
    serverinfo.port = tmp_query[i+1].substr(tmp_query[i+1].find("=") + 1);
    serverinfo.fileName = tmp_query[i+2].substr(tmp_query[i+2].find("=") + 1);
    ret.push_back(serverinfo);
  }

  return ret;
}


std::vector<ServerInfo> parseHTTPQueryString(const char *query) {
    std::vector<ServerInfo> result;
    if (query == nullptr) {
        std::cout << "query string is null\n";
        return result;
    }

    std::string queryString = query;
    std::regex regexPattern(QUERY_STRING, std::regex::optimize);
    std::regex proxyRegex(SOCKS_PROXY_QUERY_STRING, std::regex::optimize);


    std::smatch match;
    auto pos = queryString.cbegin();
    while (std::regex_search(pos, queryString.cend(), match, regexPattern)) {
        if (match[2].length() != 0 || match[3].length() != 0 || match[4].length() != 0) {
            result.emplace_back(ServerInfo{match[2], match[3], match[4]});
        }
        pos = match.suffix().first;  // Continue searching from the end of the last match
    }

    if (std::regex_search(queryString, match, proxyRegex)) {
        result.emplace_back(ServerInfo{match[1], match[2], ""});
    }

    return result;
}


std::string generateHTTPNPShellWebConsole(const std::vector<ServerInfo>& hosts) {
  std::string base_html;

  // Open the file
  std::ifstream file(NPSHELL_WEBCONSOLE);
  if (!file.is_open()) {
    std::cerr << "Failed to open the file." << std::endl;
    base_html = npshell_webconsole;
  } else {
    // Read the contents of the file into a string
    std::stringstream buffer;
    buffer << file.rdbuf(); // Send the contents of the file to the string stream
    base_html = buffer.str(); // Convert the string stream to std::string
    file.close(); // Close the file after reading
  }

  // Replace dynamic <th> tags, e.g., nplinux1.cs.nctu.edu.tw:8887
  size_t thead_pos = base_html.find("<th scope=\"col\"></th>");
  if (thead_pos != std::string::npos) {
    std::string th_tags;
    for (size_t i = 0; i < hosts.size(); ++i) {
      th_tags += "<th scope=\"col\">" + hosts[i].host + ":" + (hosts[i].port) + "</th>";
      if (i < hosts.size() - 1) th_tags += "\r\n          ";  // Add newline only between tags
    }
    base_html.replace(thead_pos, strlen("<th scope=\"col\"></th>"), th_tags);
  }

  // Replace the single <td> with multiple <td>s with incremented ids
  // e.g., <td><pre id="s0" class="mb-0"></pre></td>
  size_t tbody_pos = base_html.find("<td><pre id=\"s0\" class=\"mb-0\"></pre></td>");
  if (tbody_pos != std::string::npos) {
    std::string td_tags;
    for (size_t i = 0; i < hosts.size(); ++i) {
      td_tags += "<td><pre id=\"s" + std::to_string(i) + "\" class=\"mb-0\"></pre></td>";
      if (i < hosts.size() - 1) td_tags += "\r\n          ";  // Add newline only between tags
    }
    base_html.replace(tbody_pos, strlen("<td><pre id=\"s0\" class=\"mb-0\"></pre></td>"), td_tags);
  }

  std::ostringstream oss;

  oss << HTML_CONTENT_TYPE;
  oss << base_html;
  return oss.str();
}


std::string cgi_output_shell(std::string content, int sessionId) {
  std::string escaped_content = cgi_html_escape(content);
  return + "<script>document.getElementById(\'s" + std::to_string(sessionId) + "\').innerHTML += \'" + escaped_content + "\';</script>";
}

std::string cgi_output_command(std::string content, int sessionId) {
  std::string escaped_content = cgi_html_escape(content);
  return + "<script>document.getElementById(\'s" + std::to_string(sessionId) + "\').innerHTML += \'<b>" + escaped_content + "</b>\';</script>";
}

std::string cgi_html_escape(std::string context) {
  boost::replace_all(context, "&", "&amp;");
  boost::replace_all(context, ">", "&gt;");
  boost::replace_all(context, "<", "&lt;");
  boost::replace_all(context, "\"", "&quot;");
  boost::replace_all(context, "\'", "&apos;");
  boost::replace_all(context, "\n", "&NewLine;");
  boost::replace_all(context, "\r", "");
  return context;
}


std::string ClientInfo::output_command() {
  std::ostringstream oss;
  oss << "<script>document.getElementById('s" << sessionId << "').innerHTML += '<b>";
  printEscape(oss, userCommand);
  oss << "</b>';</script>" << std::endl;
  return oss.str();
}

std::string ClientInfo::output_shell() {
  std::ostringstream oss;
  oss << "<script>document.getElementById('s" << sessionId << "').innerHTML += '";
  printEscape(oss, shellOutput);
  oss << "';</script>" << std::endl;
  return oss.str();
}

void printEscape(std::ostringstream& out, const std::string& content) {
  for (const auto& c : content) {
    if (isalnum(c))
      out << c;
    else
      out << "&#" << static_cast<int>(c) << ";";
  }
}

std::string generatePanelTestCaseMenu(const std::string& dir) {
    std::vector<std::string> test_cases;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                test_cases.push_back(entry.path().filename().string());
            }
        }
        std::sort(test_cases.begin(), test_cases.end());
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error reading test cases: " << e.what() << std::endl;
    }

    std::ostringstream oss;
    for (const auto& test_case : test_cases) {
        oss << "<option value=\"" << test_case << "\">" << test_case << "</option>";
    }
    return oss.str();
}

std::string generatePanelHostMenu(int num_hosts, const std::string& domain) {
    std::ostringstream oss;
    for (int i = 0; i < num_hosts; ++i) {
        std::string host = "nplinux" + std::to_string(i + 1);
        oss << "<option value=\"" << host << "." << domain << "\">" << host << "</option>";
    }
    return oss.str();
}

std::string generatePanel() {
  std::ostringstream oss;
  std::string test_case_menu = generatePanelTestCaseMenu(PANEL_TEST_CASE_DIR);
  std::string host_menu = generatePanelHostMenu(PANEL_N_NP_HOST, PANEL_DOMAIN);
  oss << HTML_CONTENT_TYPE;
  oss << R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <title>NP Project 3 Panel</title>
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
      href="https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png"
    />
    <style>
      * {
        font-family: 'Source Code Pro', monospace;
      }
    </style>
  </head>
  <body class="bg-secondary pt-5">)";

    oss << R"(
    <form action=")" << PANEL_FORM_ACTION << R"(" method=")" << PANEL_FORM_METHOD << R"(">
      <table class="table mx-auto bg-light" style="width: inherit">
        <thead class="thead-dark">
          <tr>
            <th scope="col">#</th>
            <th scope="col">Host</th>
            <th scope="col">Port</th>
            <th scope="col">Input File</th>
          </tr>
        </thead>
        <tbody>)";

    for (int i = 0; i < PANEL_N_SERVERS; ++i) {
        oss << R"(
          <tr>
            <th scope="row" class="align-middle">Session )" << (i + 1) << R"(</th>
            <td>
              <div class="input-group">
                <select name="h)" << i << R"(" class="custom-select">
                  <option></option>)" << host_menu << R"(
                </select>
                <div class="input-group-append">
                  <span class="input-group-text">.cs.nycu.edu.tw</span>
                </div>
              </div>
            </td>
            <td>
              <input name="p)" << i << R"(" type="text" class="form-control" size="5" />
            </td>
            <td>
              <select name="f)" << i << R"(" class="custom-select">
                <option></option>)" << test_case_menu << R"(
              </select>
            </td>
          </tr>)";
    }

    oss << R"(
          <tr>
            <td colspan="3"></td>
            <td>
              <button type="submit" class="btn btn-info btn-block">Run</button>
            </td>
          </tr>
        </tbody>
      </table>
    </form>
  </body>
</html>)";
    return oss.str();
}