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
#include <iostream>
#include <termios.h>
#include <unistd.h>


using namespace std;

void runInteractiveMode(int socket);
// void runTestScriptMode(const string& filename, int socket);
bool sendCommandAndCheckResponse(int socket, const string& command, const vector<string>& inputs, const vector<string>& expected);
void sendMessage(int socket, const string& message);
void sendData(int socket, const string& data);

void userSend(int socket);
string autoSend(int socket, const string& receiver, const string& subject, const string& body, bool printReturn);

void userList(int socket);
string autoList(int socket, bool printReturn);

void userRead(int socket);
Message autoRead(int socket, const string& messageNr, bool printReturn);

void userDelete(int socket);
string autoDelete(int socket, const string& messageNr, bool printReturn);

bool userLogin(int socket);
int getch();
const char *getpass();


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
    bool loggedIn = false;
    string command;
    while (true) {
        if (!loggedIn) {
            cout << "Login" << endl;
            loggedIn = userLogin(socket);
        }
        else {
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
}

//TODO: TO BE IMPLEMENTED -> IGNORE FOR NOW (DEAD CODE)
// void runTestScriptMode(const string& filename, int socket) {
// }

void userSend(int socket) {
    string sender, receiver, subject, body, line;

    //Removed for PRO
    //cout << "Enter sender: ";
    //getline(cin, sender);
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
    autoSend(socket, receiver, subject, body, true);

}

string autoSend(int socket, const string& receiver, const string& subject, const string& body, bool printReturn) {
    sendMessage(socket, "SEND\n");
    sendMessage(socket, receiver + "\n");
    sendMessage(socket, subject + "\n");
    sendMessage(socket, body);
    sendMessage(socket, ".\n");

    string reply;
    readLine(socket,&reply);

    // Print reply in usermode
    //TODO: Nicer output depending on return message
    if (printReturn) cout << reply << endl;

    return reply;
}

void userList(const int socket) {
    autoList(socket, true);
}

string autoList(int socket, const bool printReturn) {

    // String containing the full return message
    string fullReply;
    string buffer;

    sendMessage(socket, "LIST\n");

    string mailAmountString;
    readLine(socket, &mailAmountString);
    const int mailAmount = stoi(mailAmountString);

    for (int i = 0; i < mailAmount; i++) {
        readLine(socket, &buffer);
        fullReply += buffer + "\n";
    }

    // Print reply in usermode
    //TODO: Nicer output depending on return message
    if (printReturn) cout << fullReply << endl;
    return fullReply;
}

void userRead(int socket) {
    string messageNumber;
    cout << "Enter message number: ";
    getline(cin, messageNumber);
    autoRead(socket, messageNumber, true);
}

Message autoRead(int socket, const string& messageNr, const bool printReturn) {
    sendMessage(socket, "READ\n");
    sendMessage(socket, messageNr+ "\n");

    Message msg;
    string status;

    readLine(socket, &status);
    if(status != "OK") {
        if (printReturn) cout << status << endl;
        return msg;
    }

    string buffer;
    readLine(socket,&msg.sender);
    readLine(socket,&msg.recipient);
    readLine(socket,&msg.subject);
    readLine(socket,&buffer);
    while(buffer != ".") {
        msg.body += buffer + "\n";
        readLine(socket,&buffer);
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
    string messageNumber;
    cout << "Enter message number: ";
    getline(cin, messageNumber);

    autoDelete(socket, messageNumber, true);
}

string autoDelete(int socket, const string& messageNr, bool printReturn) {
    sendMessage(socket, "DEL\n");
    sendMessage(socket, messageNr + "\n");

    string reply;
    readLine(socket,&reply);
    if (printReturn) cout << reply << endl;
    return reply;
}

void sendMessage(int socket, const string& message) {
    if (write(socket, message.c_str(), message.length()) == -1) {
        cerr << "Error sending message: " << message << endl;
    }
}

bool userLogin(int socket) {
    string username, password;
    char ch;

    cout << "Enter username: ";
    getline(cin, username);
    cout << "Enter password: ";
    password = getpass();

    sendMessage(socket, "LOGIN\n");
    sendMessage(socket, username + "\n");
    sendMessage(socket, password + "\n");

    string reply;
    readLine(socket, &reply);

    if (reply != "OK") {
        cout << "Login Failed!" << endl;
        return false;
    }
    cout << "Login successful!" << endl;
    return true;
}

int getch()
{
    int ch;
    // https://man7.org/linux/man-pages/man3/termios.3.html
    struct termios t_old, t_new;

    // https://man7.org/linux/man-pages/man3/termios.3.html
    // tcgetattr() gets the parameters associated with the object referred
    //   by fd and stores them in the termios structure referenced by
    //   termios_p
    tcgetattr(STDIN_FILENO, &t_old);

    // copy old to new to have a base for setting c_lflags
    t_new = t_old;

    // https://man7.org/linux/man-pages/man3/termios.3.html
    //
    // ICANON Enable canonical mode (described below).
    //   * Input is made available line by line (max 4096 chars).
    //   * In noncanonical mode input is available immediately.
    //
    // ECHO   Echo input characters.
    t_new.c_lflag &= ~(ICANON | ECHO);

    // sets the attributes
    // TCSANOW: the change occurs immediately.
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    // reset stored attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);

    return ch;
}

const char *getpass()
{
    int show_asterisk = 0;

    const char BACKSPACE = 127;
    const char RETURN = 10;

    unsigned char ch = 0;
    std::string password;

    printf("Password: ");

    while ((ch = getch()) != RETURN)
    {
        if (ch == BACKSPACE)
        {
            if (password.length() != 0)
            {
                if (show_asterisk)
                {
                    printf("\b \b"); // backslash: \b
                }
                password.resize(password.length() - 1);
            }
        }
        else
        {
            password += ch;
            if (show_asterisk)
            {
                printf("*");
            }
        }
    }
    printf("\n");
    return password.c_str();
}
