#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

typedef struct Message {
    int number;
    std::string sender;
    std::string recipient;
    std::string subject;
    std::string body;
} message_t;

message_t parseMessageFile(fs::path file);
int writeMessageFile(fs::path file, message_t message);

#endif