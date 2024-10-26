#include "socks4_util.hpp"


std::string convertToRegexPattern(std::string str) {
  // replace "." with "\."
  size_t start_pos = 0;
  while ((start_pos = str.find(".", start_pos)) != std::string::npos) {
    str.replace(start_pos, 1, "\\.");
    start_pos += 2; // Move past the newly inserted "\."
  }

  // replace "*" with ".*"
  start_pos = 0;
  while ((start_pos = str.find('*', start_pos)) != std::string::npos) {
    str.replace(start_pos, 1, ".*");
    start_pos += 2; // Move past the newly inserted ".*"
  }

  return str;
}

bool matchFirewallRule(char operation, std::string& rule, bool socks4Command, const std::string& dstIP) {
  bool configCommand = (operation == 'c');
  if ((socks4Command && configCommand) || (!socks4Command && !configCommand)) {
    std::regex ruleRegex(convertToRegexPattern(rule));
    return std::regex_match(dstIP, ruleRegex);
  }
  return false;
}