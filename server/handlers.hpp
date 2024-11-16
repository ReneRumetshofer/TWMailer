#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <string>

int handleList(int socket);
int handleSend(int socket);
int handleRead(int socket);
int handleDelete(int socket);
int readUsernameAndMessageNumber (int socket, std::string& username, int& messageNumber);

#endif