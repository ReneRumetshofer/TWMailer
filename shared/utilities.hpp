#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <string>

int readLine(int socket, std::string* receivedMessage);
int sendLine(int socket, std::string lineToSend);
bool isValidUsername(std::string username);

#endif