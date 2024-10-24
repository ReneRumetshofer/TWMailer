#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include "handlers.hpp"
#include "../shared/globals.hpp"
#include "../shared/message.hpp"
#include "../shared/utilities.hpp"
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

int handleList(int* socket) {
    string username;
    if (readLine(socket, &username) == -1) {
        return -1;
    }

    if (!isValidUsername(username)) {
        return sendLine(socket, "ERR");
    }

    // Check if user directory exists, if not send back 0 found messages.
    fs::path spoolDir(spoolPath);
    fs::path userDir = spoolDir / username;
    if (!fs::is_directory(userDir)) {
        return sendLine(socket, "0");
    }

    // Get all files in userDir
    vector<fs::path> files;
    for (const auto &entry : fs::directory_iterator(userDir)) {
        files.push_back(entry.path());
    }

    // Read all files and parse them into type message_t
    vector<message_t> messages;

    for(const auto &file : files) {
        try {
            messages.push_back(parseMessageFile(file));
        }
        catch (runtime_error &e) {
            cerr << e.what() << endl;
        }
    }

    // Message ordering descending (newest on top)
    sort(messages.begin(), messages.end(), [](const message_t& a, const message_t& b) {
        return a.number > b.number;
    });

    // Send number of total messages
    sendLine(socket,to_string(messages.size()));

    // Send individual messages
    for(const auto &message : messages) {
        string messageLine = to_string(message.number) + ": " + message.subject;
        if (sendLine(socket, messageLine) == -1) {
            return -1;
        }
    }

    return 0;
}

int handleSend(int* socket) {
    // Read and validate sender, recipient and subject (empty strings are not allowed)
    string sender;
    if (readLine(socket, &sender) == -1) {
        return -1;
    }
    if (!isValidUsername(sender)) {
        return sendLine(socket, "ERR");
    }
    string receiver;
    if (readLine(socket, &receiver) == -1) {
        return -1;
    }
    if (!isValidUsername(receiver)) {
        return sendLine(socket, "ERR");
    }
    string subject;
    if (readLine(socket, &subject) == -1) {
        return -1;
    }
    if(subject.length() > 80 || subject.empty()) {
        return sendLine(socket, "ERR");
    }

    // Until client sends ".\n" read message body lines
    string body;
    string line;
    while (line != ".") {
        if (readLine(socket, &line) == -1) {
            return -1;
        }
        if (line == ".") {
            break;
        }
        body += line + "\n";
    }

    // Check if recipient directory exists, if not create it
    fs::path spoolDir(spoolPath);
    fs::path recipientDir = spoolDir / receiver;
    if (!fs::is_directory(recipientDir)) {
        if (!fs::create_directory(recipientDir)) {
            cerr << "Error while creating recipient directory " << recipientDir.string() << endl;
            return sendLine(socket, "ERR");
        }
    }

    // Find next message number
    int nextMessageNumber = 1;
    for (const auto &entry : fs::directory_iterator(recipientDir)) {
        try {
            int messageNumber = stoi(entry.path().filename().string());
            if (messageNumber >= nextMessageNumber) {
                nextMessageNumber = messageNumber + 1;
            }
        }
        catch (invalid_argument &e) {
            cerr << "Errornous message file named " << entry.path().string() << ", file must be a number - ignoring files" << endl;
        }
    }

    // Construct message
    message_t message;
    message.number = nextMessageNumber;
    message.sender = sender;
    message.recipient = receiver;
    message.subject = subject;
    message.body = body;

    // Write message file
    fs::path messageFile = recipientDir / to_string(nextMessageNumber);
    if (writeMessageFile(messageFile, message) == -1) {
        return sendLine(socket, "ERR");
    }
    return sendLine(socket, "OK");
}

int handleRead(int* socket) {
    string username;
    int messageNumber;
    if (readUsernameAndMessageNumber(socket, username, messageNumber)  == -1) {
        return -1;
    }

    // Check if message file exists
    fs::path spoolDir(spoolPath);
    fs::path userDir = spoolDir / username;
    fs::path messageFile = userDir / to_string(messageNumber);
    if (!fs::is_regular_file(messageFile)) {
        cerr << "Cannot find message file " << messageFile.string() << " for user \"" << username << "\"" << endl;
        return sendLine(socket, "ERR");
    }

    // Read message file and send it to client
    message_t message = parseMessageFile(messageFile);
    if (sendLine(socket, "OK") == -1) {
        return -1;
    }
    if (sendLine(socket, message.sender) == -1) return -1;
    if (sendLine(socket, message.recipient) == -1) return -1;
    if (sendLine(socket, message.subject) == -1) return -1;
    if (sendLine(socket, message.body) == -1) return -1;
    
    return sendLine(socket, ".");
}

int handleDelete(int* socket) {
    string username;
    int messageNumber;
    if (readUsernameAndMessageNumber(socket, username, messageNumber)  == -1) {
        return -1;
    }

    // Check if message file exists
    fs::path spoolDir(spoolPath);
    fs::path userDir = spoolDir / username;
    fs::path messageFile = userDir / to_string(messageNumber);
    if (!fs::is_regular_file(messageFile)) {
        cerr << "Cannot find message file " << messageFile.string() << " for user \"" << username << "\"" << endl;
        return sendLine(socket, "ERR");
    }

    // Delete message file
    if (!fs::remove(messageFile)) {
        cerr << "Error while deleting message file " << messageFile.string() << endl;
        return sendLine(socket, "ERR");
    }
    
    return sendLine(socket, "OK");
}

int readUsernameAndMessageNumber (int* socket, string& username, int& messageNumber) {
    // Read and parse username
    if (readLine(socket, &username) == -1) {
        return -1;
    }
    if (!isValidUsername(username)) {
        return sendLine(socket, "ERR");
    }

    // Read message number and parse to integer
    string messageNumberRaw;
    if (readLine(socket, &messageNumberRaw) == -1) {
        return -1;
    }
    try {
        messageNumber = stoi(messageNumberRaw);
    }
    catch (invalid_argument &e) {
        cerr << "Message number must be an integer!" << endl;
        return sendLine(socket, "ERR");
    }

    return 0;
}