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
#include <algorithm>
#include <cstdio>

void defaultMethod(std::vector<std::string> messageParsed)
{
    std::cout << "METHOD: " << messageParsed[0] << std::endl;
}

void returnMessage(int socket, char *message)
{
    send(socket, message, strlen(message), 0);
}

int checkName (std::string fileName, std::string msgnumber){
    
    if (fileName != "." && fileName != ".."  && fileName != "count"){
        size_t found = fileName.find(msgnumber);
        if (found != std::string::npos) return 0; //Create a procedure in case +9 emails. For example, number at begin
    }
    return 1;
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

char *sendHandler(std::vector<std::string> message, char *dir)
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
    std::string filePath = path + "/" + message[3] + "_" + std::to_string(i) + ".txt";
    std::ofstream MailFile(filePath);
    MailFile << "SENDER: " << message[1] << std::endl;
    MailFile << "SUBJECT: " << message[3] << std::endl;
    int n = 4;
    MailFile << "CONTENT: ";
    while (message[n] != "."){
        MailFile << message[n] << std::endl;
        n++;
    }
    MailFile.close();
    return status_code[0];
}

void readHandler (std::vector<std::string> message, char *dir, int soc){
    //Identify file
    std::string path = dir;
    path += '/' + message[1];
    DIR *dp;
    std::string out = "";
    if (dp = opendir(path.c_str())){
        struct dirent *ent;
        //get subjects
        while ((ent = readdir(dp)) != NULL) {
            if (checkName (ent->d_name, message[2].c_str()) == 0) { //if it is the right file, go in
                std::ifstream content;
                content.open(path + '/' + ent->d_name);
                while (1){
                    std::string contentLine;
                    getline(content, contentLine);
                    out = out + contentLine;
                    if (content.eof()) break;
                    out = out + '\n';
                }
                getline(content, out);
                content.close();
            }
        }
        closedir(dp);
    } else {
        out = "User does not exist\n";
    }
    if (out == "") out = "No emails with that number\n";
    returnMessage(soc, (char *)out.c_str());
}

void listHandler(std::vector<std::string> message, char *dir, int soc) {
    //define path
    std::string path = dir;
    path += '/' + message[1];
    DIR *dp;
    std::string out = "";

    std::string count = "";
    if (dp = opendir(path.c_str()))
    {
        struct dirent *ent;

        //get counter
        std::ifstream is_counter;
        is_counter.open(path + "/count");
        is_counter >> count;
        is_counter.close();
        count += '\n';

        //get subjects
        while ((ent = readdir(dp)) != NULL)
        {
            std::string temp = ent->d_name;
            if (temp != "count" && temp.size() > 3)
            {
                out += temp + '\n';
            }
        }
        closedir(dp);
    }
    else
    {
        out = "0";
    }
    out.insert(0, count);
    returnMessage(soc, (char *)out.c_str());
}

void deleteHandler(std::vector<std::string> message, char *dir, int soc){
    //Identify file
    std::string path = dir;
    path += '/' + message[1];
    DIR *dp;
    std::string out = status_code[1];
    if (dp = opendir(path.c_str())){
        struct dirent *ent;
        while ((ent = readdir(dp)) != NULL) {
            if (checkName (ent->d_name, message[2].c_str()) == 0) { //if it is the right file, go in
                std::string fileName = path + "/" + ent->d_name;
                if (remove(fileName.c_str()) == 0){
                    out = status_code[0];
                    //get count down
                    break;
                }
            }
        }
        closedir(dp);
    }
    returnMessage(soc, (char *)out.c_str());
}

void mailHandler(char *input, int clientSocket, char *directory)
{
    // printf("Message received: %s", input);

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
        listHandler(messageParsed, directory, clientSocket);
    }
    else if (messageParsed[0] == "READ")
    {
        readHandler(messageParsed, directory, clientSocket);
    }
    else if (messageParsed[0] == "DEL")
    {
        deleteHandler(messageParsed, directory, clientSocket);
    }
    else
    {
        returnMessage(clientSocket, status_code[2]);
    }
}

