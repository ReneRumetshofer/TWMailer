#include "globals.hpp"
#include "utilities.hpp"
#include <string>
#include <filesystem>
#include <iostream>
#include <sys/socket.h>
#include <regex>

using namespace std;
namespace fs = std::filesystem;

int readLine(int* socket, string* receivedMessage) {
    char buffer[BUF_SIZE];

    // Read message from socket
    ssize_t size = recv(*socket, buffer, BUF_SIZE - 1, 0);
    if (size == -1) {
        if (!abortRequested) {
            cerr << "Receive error, errno is " << errno << endl;
        }
        return -1;
    }
    else if (size == 0) {
        cout << "Client closed connection." << endl;
        return -1;
    }

    // Remove newline at the end (dependant on OS)
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
        size -= 2;
    }
    else if (buffer[size - 1] == '\n') {
        --size;
    }

    buffer[size] = '\0';
    *receivedMessage = buffer;

    return 0; // Success
}

int sendLine(int* socket, string lineToSend) {
    // Convert string to c-string and send it
    lineToSend += "\n";
    if (send(*socket, lineToSend.c_str(), lineToSend.size() + 1, 0) == -1) {
        cerr << "Error while sending line \"" << lineToSend << "\"to client, errno is: " << errno << endl;
        return -1;
    }
    return 0;
}

bool isValidUsername(string username) {
    regex const usernameRegex("^[a-z0-9]{1,8}$"); 
    return regex_match(username, usernameRegex);
}