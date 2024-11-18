#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <string>
#include <mutex>
#include <map>

extern std::map<std::string, std::shared_ptr<std::mutex>> userMutexes; // Mutex per username
extern std::mutex userMutexesProtection;

std::shared_ptr<std::mutex> getUserMutex(std::string username);
int handleList(int socket, std::string loggedInUser);
int handleSend(int socket, std::string loggedInUser);
int handleRead(int socket, std::string loggedInUser);
int handleDelete(int socket, std::string loggedInUser);
int handleLogin(int socket, std::string clientIp, std::string& loggedInUser);
int readMessageNumber (int socket, int& messageNumber);

#endif