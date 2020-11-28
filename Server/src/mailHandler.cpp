#define SERVER_MAIL_HANDLER

#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include "statusCodes.cpp"
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>
#include <filesystem>

void defaultMethod(std::vector<std::string> messageParsed)
{
    std::cout << "METHOD: " << messageParsed[0] << std::endl;
}

void returnMessage(int socket, const char *message)
{

    send(socket, message, strlen(message), 0);
}

int getCount(std::string path)
{
    int i = 1;
    std::string temp;
    std::ifstream outfile(path + "/count");
    if (outfile.is_open())
    {
        getline(outfile, temp);
        i = std::stoi(temp) + 1;
        outfile.close();
    }
    std::ofstream infile(path + "/count");
    if (infile.is_open())
    {
        infile << i << "\n";
    }
    return i;
}

const char *sendHandler(std::vector<std::string> message, char *dir)
{
    // METHOD USERNAME RECEIVER SUBJECT MESSAGE
    std::string path = dir;
    path += '/' + message[2];

    if (opendir(path.c_str()) == NULL)
    {
        // read/write/search permissions for owner and group, and with read/search permissions for others
        if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        {
            perror("Failed to open/create user directory.\n.");
            return status_code[2];
        }
    }
    DIR *dp;
    int i = getCount(path);
    std::string filePath = path + "/" + std::to_string(i) + "_" + message[3];
    std::ofstream MailFile(filePath);
    MailFile << "SENDER: " << message[1] << std::endl;
    MailFile << "SUBJECT: " << message[3] << std::endl;
    MailFile << "CONTENT: " << message[4] << std::endl;
    MailFile.close();
    return status_code[0];
}

void listHandler(std::vector<std::string> message, char *dir)

{
    DIR *dp = opendir("./");

    if (dp != NULL)
    {
        return;
    }
}

void mailHandler(char *input, int clientSocket, char *directory)
{
    printf("Message received: %s", input);

    std::vector<std::string> messageParsed;
    std::string parsed;
    std::string temp(input);
    std::stringstream strm(temp);
    while (std::getline(strm, parsed))
    {
        messageParsed.push_back(parsed);
    }

    // TODO: ROUTING HERE
    if (messageParsed[0] == "SEND")
    {
        returnMessage(clientSocket, sendHandler(messageParsed, directory));
    }
    else if (messageParsed[0] == "LIST")
    {
        defaultMethod(messageParsed);
    }
    else if (messageParsed[0] == "READ")
    {
        defaultMethod(messageParsed);
    }
    else if (messageParsed[0] == "DEL")
    {
        defaultMethod(messageParsed);
    }
    else
    {
        returnMessage(clientSocket, status_code[2]);
    }
}
