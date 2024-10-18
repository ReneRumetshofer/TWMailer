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

#define MAX_BACKLOG 5 // Max number of pending connections in the listen queue
#define BUF 1024 // Message buffer size

using namespace std;
namespace fs = std::filesystem;

void printUsageAndExit();
int parsePort(char portArgument[]);
string parseSpoolPath(char spoolPathArgument[]);
void signalHandler(int signal);
void shutdownAndCloseSocket(int* socket);
void clientHandler(int* socket);

bool abortRequested = false;
int client_socket = -1;
int listening_socket = -1;

int main(int argc, char** argv) {
    // Parse arguments
    if(argc != 3) {
        printUsageAndExit();
    }
    int port = parsePort(argv[1]);
    string spoolPath = parseSpoolPath(argv[2]);

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
    while (!abortRequested) {
        cout << "Waiting for new connections.." << endl;

        // Accept new client connection, blocking
        addressLength = sizeof(struct sockaddr_in);
        if ((client_socket = accept(listening_socket,
                                (struct sockaddr *) &clientAddress,
                                &addressLength)) == -1) {
            if (abortRequested) {
                cerr << "Error in accepting client after abort was requested" << endl;
            }
            else {
                cerr << "Error while accepting new client" << endl;
            }
            cerr << "errno is " << errno << endl;
            break;
        }

        // Start client handler
        cout << "Client connected from " << inet_ntoa(clientAddress.sin_addr) << ":" 
             << ntohs(clientAddress.sin_port) << "!" << endl;
        clientHandler(&client_socket);
        client_socket = -1; // When clientHandler returned, the connection is over and this can be reset
    }

    shutdownAndCloseSocket(&listening_socket);
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
        shutdownAndCloseSocket(&listening_socket);
    }
    // Unknown signal -> exit
    else {
        exit(signal);
    }
}

void shutdownAndCloseSocket(int* socket) {
    if (*socket != -1) {
        // Shutdown initiates a TCP closing sequence (FIN) to properly close the connection
        if (shutdown(*socket, SHUT_RDWR) == -1) {
            cerr << "Error while shutting down socket." << endl;
        }
        // Close the socket entriely.
        if (close(*socket) == -1) {
            cerr << "Error while closing down socket." << endl;
        }
        *socket = -1;
    }
}

void clientHandler(int* socket) {
    char buffer[BUF];

    // SEND welcome message
    strcpy(buffer, "Welcome to myserver!\r\nPlease enter your commands...\r\n");
    if (send(*socket, buffer, strlen(buffer), 0) == -1){
        cerr << "Sending welcome message failed" << endl;
        return;
    }

    do {
        /////////////////////////////////////////////////////////////////////////
        // RECEIVE, blocks until a message comes in
        ssize_t size = recv(*socket, buffer, BUF - 1, 0);
        if (size == -1) {
            if (abortRequested) {
                cerr << "Receive error after requested abort" << endl;
            }
            else {
                cerr << "Receive error" << endl;
            }
            cerr << "errno is " << errno << endl;
            break;
        }

        if (size == 0) {
            cout << "Client closed connection." << endl;
            break;
        }

        // Remove newline at the end (dependant on OS)
        if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
            size -= 2;
        }
        else if (buffer[size - 1] == '\n') {
            --size;
        }

        buffer[size] = '\0';
        printf("Message received: %s\n", buffer); // ignore error

        if (send(*current_socket, "OK", 3, 0) == -1)
        {
            perror("send answer failed");
            return NULL;
        }
    } while (strcmp(buffer, "quit") != 0 && !abortRequested);


    shutdownAndCloseSocket(socket);
}