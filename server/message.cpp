#include "message.hpp"
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

message_t parseMessageFile(fs::path file) {
    try {
        stoi(file.filename().string());
    }
    catch (invalid_argument &e) {
        throw runtime_error("Errornous message file named " + file.string() + ", file must be a number");
    }

    ifstream fileStream(file);
    if (!fileStream.is_open()) {
        throw runtime_error("Error while opening message file " + file.string());
    }

    message_t message;
    message.number = stoi(file.filename().string());

    string line;
    if (!getline(fileStream, line)) {
        throw runtime_error("Errornous message file under " + file.string() + ", cannot find sender");
    }
    message.sender = line;

    if (!getline(fileStream, line)) {
        throw runtime_error("Errornous message file under " + file.string() + ", cannot find recipient");
    }
    message.recipient = line;

    if (!getline(fileStream, line)) {
        throw runtime_error("Errornous message file under " + file.string() + ", cannot find subject line");
    }
    message.subject = line;

    while (getline(fileStream, line)) {
        message.body += line + "\n";
    }

    return message;
}

int writeMessageFile(fs::path file, message_t message) {
    ofstream fileStream(file);
    if (!fileStream.is_open()) {
        cerr << "Error while opening message file " << file.string() << endl;
        return -1;
    }

    fileStream << message.sender << endl;
    fileStream << message.recipient << endl;
    fileStream << message.subject << endl;
    fileStream << message.body;

    return 0;
}