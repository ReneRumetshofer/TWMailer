#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <filesystem>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex>
#include <fstream>
#include <thread>
#include <mutex>
#include "handlers.hpp"
#include "blacklist.hpp"
#include "../shared/utilities.hpp"
#include "../shared/message.hpp"
#include "../shared/globals.hpp"

using namespace std;
namespace fs = std::filesystem;

void printUsageAndExit();
int parsePort(char portArgument[]);
string parseSpoolPath(char spoolPathArgument[]);
void signalHandler(int signal);
void clientHandler(int socket, string clientIp);

// Globals (as defined in globals.hpp)
bool abortRequested = false;
int client_socket = -1;
int listening_socket = -1;
string spoolPath;

int main(int argc, char** argv) {
    // Parse arguments
    if(argc != 3) {
        printUsageAndExit();
    }
    int port = parsePort(argv[1]);
    spoolPath = parseSpoolPath(argv[2]);

    // Register signal handler
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        cerr << "Error while installing signal handler!";
        exit(EXIT_FAILURE);
    }

    // Request socket from OS, set resuse addr and reuse port
    if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "Error while creating listening socket, errno is " << errno << endl;
        exit(EXIT_FAILURE);
    }
    int reuseValue = 1; // defines that reuse should be enabled
    if (setsockopt(listening_socket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &reuseValue,
                  sizeof(reuseValue)) == -1) {
        cerr << "Error while setting SO_REUSEADDR, errno is " << errno << endl;
        exit(EXIT_FAILURE);
    }
    if (setsockopt(listening_socket,
                  SOL_SOCKET,
                  SO_REUSEPORT,
                  &reuseValue,
                  sizeof(reuseValue)) == -1) {
        cerr << "Error while setting SO_REUSEPORT, errno is " << errno << endl;
        exit(EXIT_FAILURE);
    }

    // Initialize address struct and bind listening socket to this address/port combo
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(listening_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
        cerr << "Error while binding to listening socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Start listening. Maximum MAX_BACKLOG connections can be pending
    if (listen(listening_socket, MAX_BACKLOG) == -1) {
        cerr << "Error while starting to listen on listening socket" << endl;
        exit(EXIT_FAILURE);
    }

    socklen_t addressLength;
    struct sockaddr_in clientAddress;  
    vector<thread> clientThreads;
    while (!abortRequested) {
        cout << "Waiting for new connections.." << endl;

        // Accept new client connection, blocking
        addressLength = sizeof(struct sockaddr_in);
        if ((client_socket = accept(listening_socket,
                                (struct sockaddr *) &clientAddress,
                                &addressLength)) == -1) {
            if (!abortRequested) {
                cerr << "Error while accepting new client, errno is " << errno << endl;
            }
            break;
        }

        // Check blacklist
        string clientIp = inet_ntoa(clientAddress.sin_addr);
        bool isBlacklisted = isIpBlacklisted(clientIp);
        if (isBlacklisted) {
            cout << "Client IP " << clientIp << " is blacklisted, closing connection." << endl;
            shutdownAndCloseSocket(client_socket);
            continue;
        }

        // Start client handler thread
        cout << "Client connected from " << clientIp << ":" << ntohs(clientAddress.sin_port) << "!" << endl;
        thread clientThread(clientHandler, client_socket, clientIp);
        clientThread.detach(); // needed, otherwise the thread fails with an exception almost immediately
    }

    for(auto &clientThread : clientThreads) {
        clientThread.join();
    }

    shutdownAndCloseSocket(listening_socket);
    exit(EXIT_SUCCESS);
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

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "Received INT signal - shutting down server." << endl;
        abortRequested = true;
        if (client_socket != -1) {
            // Shutdown initiates a TCP closing sequence (FIN) to properly close the connection
            if (shutdown(client_socket, SHUT_RDWR) == -1) {
                cerr << "Error while shutting down active socket with a client." << endl;
            }
            // Close the socket entriely.
            if (close(client_socket) == -1) {
                cerr << "Error while closing down socket with client." << endl;
            }
            client_socket = -1;
        }

        // Shutdown & close listening socket aswell
        shutdownAndCloseSocket(listening_socket);
    }
    // Unknown signal -> exit
    else {
        cerr << "Received signal " << signal << ", exiting." << endl;
        exit(signal);
    }
}

void clientHandler(int socket, string clientIp) {
    string receivedCommand;
    string loggedInUser;
    while (!abortRequested) {
        if (readLine(socket, &receivedCommand) == -1) {
            break;
        }

        // Only allow mail commands if logged in
        if (!loggedInUser.empty()) {
            if (receivedCommand == "LIST") {
                handleList(socket, loggedInUser);
            }
            else if (receivedCommand == "SEND") {
                handleSend(socket, loggedInUser);
            }
            else if(receivedCommand == "READ") {
                handleRead(socket, loggedInUser);
            }
            else if(receivedCommand == "DEL") {
                handleDelete(socket, loggedInUser);
            }
            else if(receivedCommand == "LOGOUT") {
                loggedInUser.clear();
                cout << "Client has logged out." << endl;
                sendLine(socket, "OK");
            }
            else if(receivedCommand == "QUIT") {
                cout << "Client has sent QUIT, closing connection." << endl;
                break;
            }
            else {
                cerr << "Unknown command from client: " << receivedCommand << endl;
                sendLine(socket, "ERR");
            }
        }
        // Only allow LOGIN command if not logged in
        else {
            if (receivedCommand == "LOGIN") {
                int handleLoginResult = handleLogin(socket, clientIp, loggedInUser);
                if (handleLoginResult == -1) {
                    break;
                }

                if (!loggedInUser.empty()) {
                    cout << loggedInUser << " has logged in." << endl;
                }
            }
            else if(receivedCommand == "QUIT") {
                cout << "Client has sent QUIT, closing connection." << endl;
                break;
            }
            else {
                cerr << "Client not logged in, refusing command: " << receivedCommand << endl;
                sendLine(socket, "ERR");
            }
        }
    };

    shutdownAndCloseSocket(socket);
}