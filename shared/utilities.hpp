#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <string>

int readLine(int socket, std::string* receivedMessage);
int sendLine(int socket, std::string lineToSend);
void shutdownAndCloseSocket(int socket);
bool isValidUsername(std::string username);

#endif