#ifndef SOCKS4_UTIL_HPP
#define SOCKS4_UTIL_HPP
#include <regex>
#include <iostream>

#define SOCKS4_VERSION 0
#define SOCKS4_CMD_CONNECT 1
#define SOCKS4_CMD_BIND 2
#define SOCKS4_REPLY_GRANTED 90
#define SOCKS4_REPLY_REJECTED 91


static std::regex socks4a(R"(0\.0\.0\.[1-9][0-9]{0,2})", std::regex_constants::optimize);

#pragma pack(push)
#pragma pack(1)
struct Socks4Header {
  uint8_t version;
  uint8_t command;
  uint16_t dstport;
  uint32_t dstaddr;
};
#pragma pack(pop)

std::string convertToRegexPattern(std::string str);
bool matchFirewallRule(char operation, std::string& rule, bool socks4Command, const std::string& dstIP);

#endif // SOCKS4_UTIL_HPP