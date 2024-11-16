#include "globals.hpp"
#include "utilities.hpp"
#include <string>
#include <filesystem>
#include <iostream>
#include <sys/socket.h>
#include <regex>

using namespace std;
namespace fs = std::filesystem;

int readLine(int socket, string* receivedMessage) {
    char buffer;
    ssize_t bytesRead;

    receivedMessage->clear();  // Clear the message before reading new data

    // Reading the whole buffer (for example 1024 bytes max) does not work, as TCP can put to message lines into one packet that would need to be read separately
    while (true) {
        // Read one byte at a time from the socket
        bytesRead = recv(socket, &buffer, 1, 0); 

        // When recv returns -1 -> an error occurred
        if (bytesRead == -1) {
            cerr << "Error reading from socket, errno is " << errno << endl;
            return -1; 
        } 
        else if (bytesRead == 0) {
            cout << "Client closed connection unexpectedly" << endl;
            return -1;
        }

        // Only when newline character is found, stop reading bytes
        if (buffer == '\n') {
            break;
        }

        // Append the newly received character to the receivedMessage
        receivedMessage->append(1, buffer);
    }

    return 0;
}

int sendLine(int socket, string lineToSend) {
    // Convert string to c-string and send it
    lineToSend += "\n";
    if (send(socket, lineToSend.c_str(), lineToSend.size(), 0) == -1) {
        cerr << "Error while sending line \"" << lineToSend << "\"to client, errno is: " << errno << endl;
        return -1;
    }
    return 0;
}

bool isValidUsername(string username) {
    regex const usernameRegex("^[a-z0-9]{1,8}$"); 
    return regex_match(username, usernameRegex);
}