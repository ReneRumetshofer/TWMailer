#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
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