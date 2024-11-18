#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string>

#define MESSAGES_DIR "messages"
#define BLACKLIST_DIR "blacklist"
#define LDAP_URL "ldap://ldap.technikum-wien.at:389"
#define LDAP_USER_PATH ",ou=people,dc=technikum-wien,dc=at"

constexpr int BUF_SIZE = 1024; 
constexpr int MAX_BACKLOG  = 5; // Max number of pending connections in the listen queue
extern bool abortRequested;
extern int client_socket;
extern int listening_socket;
extern std::string spoolPath;

#endif 