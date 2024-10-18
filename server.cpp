#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

void printUsageAndExit();
int parsePort(char portArgument[]);
string parseSpoolPath(char spoolPathArgument[]);

int main(int argc, char** argv) {
    if(argc != 3) {
        printUsageAndExit();
    }
    int port = parsePort(argv[1]);
    string spoolPath = parseSpoolPath(argv[2]);

    cout << "Port: " << port << endl;
    cout << "Spool path: " << spoolPath << endl;
}

void printUsageAndExit() {
    cerr << "Usage: server <port> <mail-spool-directory>" << endl;
    cerr << "Parameters:" << endl;
    cerr << "  <port>: Port on which the mail server listens" << endl;
    cerr << "  <mail-spool-directory>: Directory where the mail gets stored" << endl;
    exit(EXIT_FAILURE);
}

int parsePort(char portArgument[]) {
    string portRaw = string(portArgument);
    int port;
    try {
        port = stoi(portRaw);
    } catch (invalid_argument &e) {
        cerr << "Port must be a number!" << endl;
        exit(EXIT_FAILURE);
    }

    if(port <= 0 || port > 65535) {
        cerr << "Port must be between 1 and 65535!" << endl;
        exit(EXIT_FAILURE);
    }

    return port;
}

string parseSpoolPath(char spoolPathArgument[]) {
    string spoolPath = string(spoolPathArgument);
    if(!fs::is_directory(spoolPath)) {
        cerr << "Error: " << spoolPath << " is not a valid path to a directory" << endl;
        exit(EXIT_FAILURE);
    }

    return spoolPath;
}