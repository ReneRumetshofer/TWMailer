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
    char buffer;
    ssize_t bytesRead;

    receivedMessage->clear();  // Clear the message before reading new data

    while (true) {
        // Read one byte at a time from the socket
        bytesRead = recv(*socket, &buffer, 1, 0);  // Use 0 for no special flags

        if (bytesRead == -1) {
            cerr << "Error reading from socket with " << endl;
            return -1;  // Return -1 on error
        } else if (bytesRead == 0) {
            cout << "Connection closed." << endl;
            return -1;  // Return -1 if the connection is closed
        }

        if (buffer == '\n') {
            // Newline encountered, stop reading and return success
            break;
        }

        // Append the character to the receivedMessage
        receivedMessage->append(1, buffer);
    }

    return 0;  // Return 0 to indicate success
}

int sendLine(int* socket, string lineToSend) {
    // Convert string to c-string and send it
    lineToSend += "\n";
    if (send(*socket, lineToSend.c_str(), lineToSend.size(), 0) == -1) {
        cerr << "Error while sending line \"" << lineToSend << "\"to client, errno is: " << errno << endl;
        return -1;
    }
    return 0;
}

bool isValidUsername(string username) {
    regex const usernameRegex("^[a-z0-9]{1,8}$"); 
    return regex_match(username, usernameRegex);
}