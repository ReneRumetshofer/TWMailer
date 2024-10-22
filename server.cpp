#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <filesystem>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex>
#include <fstream>

#define MAX_BACKLOG 5 // Max number of pending connections in the listen queue
#define BUF 1024 // Message buffer size

using namespace std;
namespace fs = std::filesystem;

typedef struct Message {
    int number;
    string sender;
    string recipient;
    string subject;
    string body;
} message_t;

void printUsageAndExit();
int parsePort(char portArgument[]);
string parseSpoolPath(char spoolPathArgument[]);
void signalHandler(int signal);
void shutdownAndCloseSocket(int* socket);
void clientHandler(int* socket);
int readLine(int* socket, string* receivedMessage);
int sendLine(int* socket, string lineToSend);

int handleList(int* socket);
int handleSend(int* socket);
int handleRead(int* socket);
int handleDelete(int* socket);
bool isValidUsername(string username);
message_t parseMessageFile(fs::path file);
int writeMessageFile(fs::path file, message_t message);

bool abortRequested = false;
int client_socket = -1;
int listening_socket = -1;
string spoolPath;

int main(int argc, char** argv) {
    // Parse arguments
    if(argc != 3) {
        printUsageAndExit();
    }
    int port = parsePort(argv[1]);
    spoolPath = parseSpoolPath(argv[2]);

    // Register signal handler
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        cerr << "Error while installing signal handler!";
        exit(EXIT_FAILURE);
    }

    // Request socket from OS, set resuse addr and reuse port
    if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "Error while creating listening socket, errno is " << errno << endl;
        exit(EXIT_FAILURE);
    }
    int reuseValue = 1; // defines that reuse should be enabled
    if (setsockopt(listening_socket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &reuseValue,
                  sizeof(reuseValue)) == -1) {
        cerr << "Error while setting SO_REUSEADDR, errno is " << errno << endl;
        exit(EXIT_FAILURE);
    }
    if (setsockopt(listening_socket,
                  SOL_SOCKET,
                  SO_REUSEPORT,
                  &reuseValue,
                  sizeof(reuseValue)) == -1) {
        cerr << "Error while setting SO_REUSEPORT, errno is " << errno << endl;
        exit(EXIT_FAILURE);
    }

    // Initialize address struct and bind listening socket to this address/port combo
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(listening_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
        cerr << "Error while binding to listening socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Start listening. Maximum MAX_BACKLOG connections can be pending
    if (listen(listening_socket, MAX_BACKLOG) == -1) {
        cerr << "Error while starting to listen on listening socket" << endl;
        exit(EXIT_FAILURE);
    }

    socklen_t addressLength;
    struct sockaddr_in clientAddress;  
    while (!abortRequested) {
        cout << "Waiting for new connections.." << endl;

        // Accept new client connection, blocking
        addressLength = sizeof(struct sockaddr_in);
        if ((client_socket = accept(listening_socket,
                                (struct sockaddr *) &clientAddress,
                                &addressLength)) == -1) {
            if (!abortRequested) {
                cerr << "Error while accepting new client, errno is " << errno << endl;
            }
            break;
        }

        // Start client handler
        cout << "Client connected from " << inet_ntoa(clientAddress.sin_addr) << ":" 
             << ntohs(clientAddress.sin_port) << "!" << endl;
        clientHandler(&client_socket);
        client_socket = -1; // When clientHandler returned, the connection is over and this can be reset
    }

    shutdownAndCloseSocket(&listening_socket);
    exit(EXIT_SUCCESS);
}

void printUsageAndExit() {
    cerr << "Usage: server <port> <mail-spool-directory>" << endl;
    cerr << "Parameters:" << endl;
    cerr << "  <port>: Port on which the mail server listens" << endl;
    cerr << "  <mail-spool-directory>: Directory where the mail gets stored" << endl;
    exit(EXIT_FAILURE);
}

int parsePort(char portArgument[]) {
    string portRaw = string(portArgument);
    int port;
    try {
        port = stoi(portRaw);
    } catch (invalid_argument &e) {
        cerr << "Port must be a number!" << endl;
        exit(EXIT_FAILURE);
    }

    if(port <= 0 || port > 65535) {
        cerr << "Port must be between 1 and 65535!" << endl;
        exit(EXIT_FAILURE);
    }

    return port;
}

string parseSpoolPath(char spoolPathArgument[]) {
    string spoolPath = string(spoolPathArgument);
    if(!fs::is_directory(spoolPath)) {
        cerr << "Error: " << spoolPath << " is not a valid path to a directory" << endl;
        exit(EXIT_FAILURE);
    }

    return spoolPath;
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "Received INT signal - shutting down server." << endl;
        abortRequested = true;
        if (client_socket != -1) {
            // Shutdown initiates a TCP closing sequence (FIN) to properly close the connection
            if (shutdown(client_socket, SHUT_RDWR) == -1) {
                cerr << "Error while shutting down active socket with a client." << endl;
            }
            // Close the socket entriely.
            if (close(client_socket) == -1) {
                cerr << "Error while closing down socket with client." << endl;
            }
            client_socket = -1;
        }

        // Shutdown & close listening socket aswell
        shutdownAndCloseSocket(&listening_socket);
    }
    // Unknown signal -> exit
    else {
        cerr << "Received signal " << signal << ", exiting." << endl;
        exit(signal);
    }
}

void shutdownAndCloseSocket(int* socket) {
    if (*socket != -1) {
        // Shutdown initiates a TCP closing sequence (FIN) to properly close the connection
        if (shutdown(*socket, SHUT_RDWR) == -1) {
            cerr << "Error while shutting down socket." << endl;
        }
        // Close the socket entriely.
        if (close(*socket) == -1) {
            cerr << "Error while closing down socket." << endl;
        }
        *socket = -1;
    }
}

void clientHandler(int* socket) {
    string receivedCommand;
    while (!abortRequested) {
        if (readLine(socket, &receivedCommand) == -1) {
            break;
        }

        // Handle different use cases
        if (receivedCommand == "LIST") {
            cout << "Handling LIST" << endl;
            handleList(socket);
        }
        else if (receivedCommand == "SEND") {
            cout << "Handling SEND" << endl;
            handleSend(socket);
        }
        else if(receivedCommand == "READ") {
            cout << "Handling READ" << endl;
            handleRead(socket);
        }
        else if(receivedCommand == "DEL") {
            cout << "Handling DEL" << endl;
            handleDelete(socket);
        }
        else if(receivedCommand == "QUIT") {
            cout << "Client has sent QUIT, closing connection." << endl;
            break;
        }
        else {
            cerr << "Unknown command from client: " << receivedCommand << endl;
            sendLine(socket, "ERR");
        }
    };

    shutdownAndCloseSocket(socket);
}

int readLine(int* socket, string* receivedMessage) {
    char buffer[BUF];

    // Read message from socket
    ssize_t size = recv(*socket, buffer, BUF - 1, 0);
    if (size == -1) {
        if (!abortRequested) {
            cerr << "Receive error, errno is " << errno << endl;
        }
        return -1;
    }
    else if (size == 0) {
        cout << "Client closed connection." << endl;
        return -1;
    }

    // Remove newline at the end (dependant on OS)
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
        size -= 2;
    }
    else if (buffer[size - 1] == '\n') {
        --size;
    }

    buffer[size] = '\0';
    *receivedMessage = buffer;

    return 0; // Success
}

int sendLine(int* socket, string lineToSend) {
    // Convert string to c-string and send it
    lineToSend += "\n";
    if (send(*socket, lineToSend.c_str(), lineToSend.size() + 1, 0) == -1) {
        cerr << "Error while sending line \"" << lineToSend << "\"to client, errno is: " << errno << endl;
        return -1;
    }
    return 0;
}

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
    if (sendLine(socket, message.sender) == -1) {
        return -1;
    }
    if (sendLine(socket, message.recipient) == -1) {
        return -1;
    }
    if (sendLine(socket, message.subject) == -1) {
        return -1;
    }
    return sendLine(socket, message.body);
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

bool isValidUsername(string username) {
    regex const usernameRegex("^[a-z0-9]{1,8}$"); 
    return regex_match(username, usernameRegex);
}