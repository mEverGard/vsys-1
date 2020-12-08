
//CONSTANTS
#define BUF 1024

//IMPORTS
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <termios.h>
#include <stdio.h>
#include "src/encryption.cpp"

std::string enterKey(){
    std::string key = "";
    while (key.length() != 4){
        if (key.length() > 0) std::cout << "Key must have 4 digits" << std::endl;
        std::cout << "Please enter the key: ";
        getline(std::cin, key);
        if (key == "0") break;
    }
    return key;
}

std::string get_pass()
{
    termios old;
    std::string pw;
    struct termios term;
    tcgetattr(STDIN_FILENO, &old);
    termios newTer = old;
    newTer.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newTer);

    std::getline(std::cin >> std::ws, pw);

    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return pw;
}

std::string getInput(std::string output, bool isMessage = false)
{
    printf("%s", output.c_str());
    std::string input;
    if (!isMessage)
    {
        std::getline(std::cin, input);
        return input + "\n";
    }
    std::string bodyline;
    while (bodyline != ".")
    {
        std::getline(std::cin, bodyline);
        input += bodyline + '\n';
    }
    return input;
}

int main(int argc, char *argv[])
{

    //1 - INITIALIZATION

    //Variables
    char ipAddress[20];
    int socketPort;

    //Getopt section
    if (argc != 5)
    {
        printf("Usage: %s -i IPAdress -p Port\n", argv[0]);
        return 1;
    }

    int c;
    while ((c = getopt(argc, argv, "i:p:")) != EOF)
    {
        switch (c)
        {
        case 'i':
            //Validation pending
            strcpy(ipAddress, optarg);
            break;
        case 'p':
            //Validation pending
            socketPort = atoi(optarg);
            break;
        case '?':
            printf("Not valid options");
            break;
        default:
            assert(0);
            break;
        }
    }

    //2 - CONNECT
    int create_socket;
    char buffer[BUF];
    struct sockaddr_in address;
    int size;

    //Define socket
    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error");
        return 1;
    }

    //Setting up the address
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;                     //format ipV4
    address.sin_port = htons(socketPort);             //port
    inet_pton(AF_INET, ipAddress, &address.sin_addr); //IP

    //Connection
    if (connect(create_socket, (struct sockaddr *)&address, sizeof(address)) == 0)
    {
        printf("Connection with server (%s) established\n", inet_ntoa(address.sin_addr));
        size = recv(create_socket, buffer, BUF - 1, 0);
        if (size > 0)
        {
            buffer[size] = '\0';
            printf("%s", buffer);
        }
    }
    else
    {
        perror("Connect error - no server available");
        return 1;
    }

    //Send message

    //Login procedure
    int loggedIn = 0;
    std::string username = "";
    do
    {
        std::string message = "";
        std::cout << "Username: ";
        std::getline(std::cin, username);
        username = username + '\n';
        message.append(username);
        std::cout << "Password: ";
        std::string password(get_pass());
        message.append(password);

        send(create_socket, message.c_str(), strlen(message.c_str()), 0);
        size = recv(create_socket, buffer, BUF - 1, 0);
        if (size > 0)
        {
            buffer[size] = '\0';
            if (strcmp(buffer, "Login succesful.\n") == 0)
                loggedIn = 1;
            printf("%s", buffer);
        }
    } while (loggedIn == 0);

    do
    {
        std::string message;
        std::string method = getInput("METHOD: ");
        message.append(method);

        //SEND Instruction
        if (method.compare("SEND\n") == 0)
        {
            int usercount = 1;

            //Receivers input
            std::string addusers = getInput("Additional receivers (y/n): ");
            message.append(addusers);
            if (addusers == "y\n")
            {
                std::string userinput = "";
                while (userinput != ".\n")
                {
                    userinput = getInput("Receiver " + std::to_string(usercount) + ": ");
                    usercount++;
                    message.append(userinput);
                }
            }
            else
            {
                message.append(getInput("Receiver: "));
            }
            //Message
            std::string encryption;
            message.append(getInput("Subject: "));
            std::cout << "Encryption? (y/n): ";
            getline(std::cin,encryption);
            if (encryption == "y"){
                std::string key = enterKey();
                message.append(encryptEmail(getInput("Message: ", true),key));
            } else {
                if (encryption != "n") std::cout << "Input not recognised, email will not be encrypted";
                message.append(getInput("Message: ", true));
            }
        }
        else if (method.compare("READ\n") == 0 || method.compare("DEL\n") == 0)
        {
            message.append(getInput("Message Number: "));
        }
        else if (method.compare("LIST\n") == 1 && method.compare("QUIT\n") == 1)
        {
            printf("\033[0;31mINVALID METHOD\033[0m\n");
            continue;
        }

        strcpy(buffer, message.c_str());
        send(create_socket, buffer, strlen(buffer), 0);
        size = recv(create_socket, buffer, BUF - 1, 0);
        if (size > 0)
        {
            if (method == "READ\n"){
                std::string key = enterKey();
                std::string message(buffer);
                int conPos = message.find("CONTENT: ");
                std::string content = decryptEmail(message.substr(conPos+ 9, std::string::npos), key);
                message.replace(conPos + 9, std::string::npos, content);
                strcpy(buffer, message.c_str());
            }
            buffer[size] = '\0';
            printf("\033[0;32m%s\033[0m\n", buffer);
        }
        else if (size == 0)
        {
            printf("Server said Bye Bye.\n");
            fflush(stdout);
            close(create_socket);
            exit(0);
        }
    } while (strcmp(buffer, "quit\n") != 0);
    close(create_socket);

    return 0;
}