#ifndef BLACKLIST_HPP
#define BLACKLIST_HPP

#include <string>
#include <map>
#include <mutex>
#include <filesystem>

#define BLACKLIST_SECONDS 60
#define BLACKLIST_LOGIN_ATTEMPTS 3

extern std::map<std::string, int> failedLoginAttempts; // Failures per IP
extern std::mutex failedLoginAttemptsMutex;

bool isIpBlacklisted(std::string ip);
bool recordFailedLogin(std::string clientIp);
void resetFailedLoginAttempts(std::string clientIp);

#endif