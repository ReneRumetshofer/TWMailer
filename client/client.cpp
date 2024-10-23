#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "../shared/message.hpp"
#include "../shared/utilities.hpp"

using namespace std;

void runInteractiveMode(int socket);
// void runTestScriptMode(const string& filename, int socket);
bool sendCommandAndCheckResponse(int socket, const string& command, const vector<string>& inputs, const vector<string>& expected);
void sendMessage(int socket, const string& message);
void sendData(int socket, const string& data);

void userSend(int socket);
string autoSend(int socket, const string& sender, const string& receiver, const string& subject, const string& body, bool printReturn);

void userList(int socket);
string autoList(int socket, const string& username, bool printReturn);

void userRead(int socket);
Message autoRead(int socket, const string& username, const string& messageNr, bool printReturn);

void userDelete(int socket);
string autoDelete(int socket, const string& username, const string& messageNr, bool printReturn);

int readLine(int* socket, string* receivedMessage);

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <server_ip> <port> [-t <test_script>]" << endl;
        return 1;
    }

    string serverIp = argv[1];
    int port;
    try {
        port = stoi(argv[2]);
    } catch (invalid_argument &e) {
        cerr << "Invalid port number: " << argv[2] << endl;
        return 1;
    }

    // Check for optional test mode
    bool testMode = false;
    string scriptFile;

    if (argc > 3 && strcmp(argv[3], "-t") == 0) {
        testMode = true;
        if (argc != 5) {
            cerr << "Usage: " << argv[0] << " <server_ip> <port> [-t <test_script>]" << endl;
            return 1;
        }
        scriptFile = argv[4];
    }

    // Socket setup
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        cerr << "Error creating socket" << endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);  // Use the provided port number

    if (inet_pton(AF_INET, serverIp.c_str(), &server_addr.sin_addr) <= 0) { // Convert IP address
        cerr << "Invalid address or address not supported" << endl;
        close(client_socket);
        return -1;
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Error connecting to server" << endl;
        close(client_socket);
        return -1;
    }

    if (testMode) {
        // runTestScriptMode(scriptFile, client_socket);
        cout << "Test mode not implemented yet." << endl;
    } else {
        runInteractiveMode(client_socket);
    }

    close(client_socket);
    return 0;
}



void runInteractiveMode(int socket) {
    string command;
    while (true) {
        cout << "Enter command (SEND, LIST, READ, DEL, QUIT): ";
        getline(cin, command);

        if (command == "QUIT") {
            sendMessage(socket, "QUIT\n");
            break;  // Quit and close the socket
        }

        if (command == "SEND") {
            userSend(socket);
        }
        else if (command == "LIST") {
            userList(socket);
        }
        else if (command == "READ") {
            userRead(socket);
        }
        else if (command == "DEL") {
            userDelete(socket);
        }
        else {
            cerr << "Unknown command!" << endl;
            continue;
        }
    }
}

//TODO: TO BE IMPLEMENTED -> IGNORE FOR NOW (DEAD CODE)
// void runTestScriptMode(const string& filename, int socket) {
// }

void userSend(int socket) {
    string sender, receiver, subject, body, line;

    cout << "Enter sender: ";
    getline(cin, sender);
    cout << "Enter receiver: ";
    getline(cin, receiver);
    cout << "Enter subject (max 80 chars): ";
    getline(cin, subject);
    if (subject.length() > 80) {
        cerr << "Subject is too long (max 80 chars)." << endl;
        return;
    }

    cout << "Enter message body (end with a single '.' on a new line): " << endl;
    while (true) {
        getline(cin, line);
        if (line == ".") {
            break;
        }
        body += line + "\n";
    }
    autoSend(socket, sender, receiver, subject, body, true);

}

string autoSend(int socket, const string& sender, const string& receiver, const string& subject, const string& body, bool printReturn) {
    sendMessage(socket, "SEND\n");
    sendMessage(socket, sender + "\n");
    sendMessage(socket, receiver + "\n");
    sendMessage(socket, subject + "\n");
    sendMessage(socket, body);
    sendMessage(socket, ".\n");

    string reply;
    readLine(&socket,&reply);

    // Print reply in usermode
    //TODO: Nicer output depending on return message
    if (printReturn) cout << reply << endl;

    return reply;
}

void userList(const int socket) {
    string username;
    cout << "Enter username: ";
    getline(cin, username);


    autoList(socket, username, true);
}

string autoList(int socket, const string& username, const bool printReturn) {

    // String containing the full return message
    string fullReply;
    string buffer;

    sendMessage(socket, "LIST\n");
    sendMessage(socket, username + "\n");

    string mailAmountString;
    readLine(&socket, &mailAmountString);
    const int mailAmount = stoi(mailAmountString);

    for (int i = 0; i < mailAmount; i++) {
        readLine(&socket, &buffer);
        fullReply += buffer + "\n";
    }

    // Print reply in usermode
    //TODO: Nicer output depending on return message
    if (printReturn) cout << fullReply << endl;
    return fullReply;
}

void userRead(int socket) {
    string username, messageNumber;
    cout << "Enter username: ";
    getline(cin, username);
    cout << "Enter message number: ";
    getline(cin, messageNumber);
    autoRead(socket, username, messageNumber, true);
}

Message autoRead(int socket, const string& username, const string& messageNr, const bool printReturn) {
    sendMessage(socket, "READ\n");
    sendMessage(socket, username + "\n");
    sendMessage(socket, messageNr+ "\n");

    Message msg;
    string status;

    readLine(&socket, &status);
    if(status != "OK") {
        if (printReturn) cout << status << endl;
        return msg;
    }

    string buffer;
    readLine(&socket,&msg.sender);
    readLine(&socket,&msg.recipient);
    readLine(&socket,&msg.subject);
    readLine(&socket,&buffer);
    while(buffer != ".") {
        msg.body += buffer + "\n";
        readLine(&socket,&buffer);
    }

    if (printReturn) {
        cout << "-------------------------------------------------------" << endl;
        cout << "FROM:  " + msg.sender << endl;
        cout << "TO:    " + msg.recipient << endl;
        cout << "-------------------------------------------------------" << endl;
        cout << "Subject:   " + msg.subject << endl;
        cout << "-------------------------------------------------------" << endl;
        cout << msg.body;
        cout << "-------------------------------------------------------" << endl;
    }
    return msg;
}

void userDelete(int socket) {
    string username, messageNumber;
    cout << "Enter username: ";
    getline(cin, username);
    cout << "Enter message number: ";
    getline(cin, messageNumber);

    autoDelete(socket, username, messageNumber, true);
}

string autoDelete(int socket, const string& username, const string& messageNr, bool printReturn) {
    sendMessage(socket, "DEL\n");
    sendMessage(socket, username + "\n");
    sendMessage(socket, messageNr + "\n");

    string reply;
    readLine(&socket,&reply);
    if (printReturn) cout << reply << endl;
    return reply;
}

void sendMessage(int socket, const string& message) {
    if (write(socket, message.c_str(), message.length()) == -1) {
        cerr << "Error sending message: " << message << endl;
    }
}