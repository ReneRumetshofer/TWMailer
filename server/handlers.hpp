#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <string>

int handleList(int socket, std::string loggedInUser);
int handleSend(int socket, std::string loggedInUser);
int handleRead(int socket, std::string loggedInUser);
int handleDelete(int socket, std::string loggedInUser);
int handleLogin(int socket, std::string clientIp, std::string& loggedInUser);
int readMessageNumber (int socket, int& messageNumber);

#endif