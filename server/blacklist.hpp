#ifndef BLACKLIST_HPP
#define BLACKLIST_HPP

#include <string>
#include <filesystem>

#define BLACKLIST_SECONDS 60

bool isIpBlacklisted(std::string ip);

#endif