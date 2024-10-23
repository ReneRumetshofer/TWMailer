#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string>

constexpr int BUF_SIZE = 1024; 
constexpr int MAX_BACKLOG  = 5; // Max number of pending connections in the listen queue
extern bool abortRequested;
extern int client_socket;
extern int listening_socket;
extern std::string spoolPath;

#endif 