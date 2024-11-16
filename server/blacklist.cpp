#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include "blacklist.hpp"
#include "../shared/globals.hpp"

using namespace std;
namespace fs = std::filesystem;

bool isIpBlacklisted(std::string ip) {
    fs::path spoolDir(spoolPath);
    fs::path blacklistDir = spoolDir / BLACKLIST_DIR;
    fs::path ipBlacklistFile = blacklistDir / ip;
    if (!fs::is_regular_file(ipBlacklistFile)) {
        return false;
    }

    // Read the file and check the saved timestamp
    string timestampRaw;
    ifstream file(ipBlacklistFile);
    getline(file, timestampRaw);
    file.close();

    // Check if the blacklist timestamp is before now
    time_t currentTime = time(nullptr);
    time_t blacklistTime;
    try {
        blacklistTime = stol(timestampRaw);
    } catch (invalid_argument &e) {
        cerr << "Timestamp in file " << ipBlacklistFile << "has invalid content: " << timestampRaw << endl;
        return false;
    }

    return currentTime < blacklistTime;
}

map<string, int> failedLoginAttempts; // Failures per IP
mutex failedLoginAttemptsMutex;

// Return true if the IP is blacklisted
bool recordFailedLogin(string clientIp) {
    //  Check if the IP is already blacklisted -> avoid case where two clients from same IP are used
    if (isIpBlacklisted(clientIp)) {
        return true;
    }

    std::lock_guard<std::mutex> lock(failedLoginAttemptsMutex);
    
    failedLoginAttempts[clientIp]++;
    if(failedLoginAttempts[clientIp] >= BLACKLIST_LOGIN_ATTEMPTS) {
        fs::path spoolDir(spoolPath);
        fs::path blacklistDir = spoolDir / BLACKLIST_DIR;
        fs::path ipBlacklistFile = blacklistDir / clientIp;

        ofstream file(ipBlacklistFile);
        file << to_string(time(nullptr) + BLACKLIST_SECONDS) << endl;
        file.close();
        failedLoginAttempts[clientIp] = 0;

        cout << "IP " << clientIp << " has been blacklisted for " << BLACKLIST_SECONDS << " seconds." << endl;

        return true;
    }
    return false;
}

void resetFailedLoginAttempts(string clientIp) {
    std::lock_guard<std::mutex> lock(failedLoginAttemptsMutex);
    failedLoginAttempts[clientIp]  = 0;
}