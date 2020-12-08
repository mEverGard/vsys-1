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

pthread_mutex_t _mutex;

int checkName(std::string fileName, std::string msgnumber)
{

    if (fileName != "." && fileName != ".." && fileName != "index")
    {
        size_t found = fileName.find(msgnumber);
        if (found != std::string::npos)
            return 0; //Create a procedure in case +9 emails. For example, number at begin
    }
    return 1;
}

bool checkLength(std::string str, int max)
{
    int size = (int)str.size();
    if (size > max || size < 1)
    {
        return true;
    }
    return false;
}

int getIndex(std::string path)
{
    int i = 1;
    std::string temp;
    std::ifstream outfile(path + "/index");
    if (outfile.is_open())
    {
        getline(outfile, temp);
        i = std::stoi(temp) + 1;
        outfile.close();
    }
    std::ofstream infile(path + "/index");
    if (infile.is_open())
    {
        infile << i << "\n";
    }
    return i;
}

int saveEmail(std::string username, std::vector<std::string> message, int subjectRow, std::string receiver, char *dir, int soc, int main, bool sent)
{

    std::string out = status_code[1];

    //define where it is saved
    std::string path = dir;

    if (!checkLength(receiver, 8) || !checkLength(message[2], 80))
    {
        send(soc, status_code[3], strlen(status_code[3]), 0);
        return 0;
    }

    path += '/' + receiver;

inFolder:
    if (opendir(path.c_str()) == NULL)
    {
        // read/write/search permissions for owner and group, and with read/search permissions for others
        if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        {
            perror("Failed to open/create user directory.\n.");
            return (-1);
        }
    }
    if (sent == true)
    {
        path += "/sent";
        sent = false;
        goto inFolder;
    }

    //send code back
    int i = getIndex(path);
    int mainIndex;
    std::string filePath = path + "/" + std::to_string(i) + "_" + message[subjectRow] + ".txt";

    std::ofstream MailFile(filePath);
    if (main == 0)
    {
        MailFile << "SENDER: " << username << std::endl;
        int n = 2;
        MailFile << "RECEIVER: ";
        if (subjectRow == 3)
            MailFile << message[n] << std::endl;
        while ((message[n] != ".") && subjectRow > 3)
        {
            MailFile << message[n] << std::endl;
            n++;
        }
        MailFile << "SUBJECT: " << message[subjectRow] << std::endl;
        n = subjectRow + 1;
        MailFile << "CONTENT: ";
        while (message[n] != ".")
        {
            MailFile << message[n] << std::endl;
            n++;
        }
        mainIndex = i;
    }
    else if (main > 0)
    {
        MailFile << "***" << std::endl;
        std::string origpath = dir;
        origpath += "/Pool/" + std::to_string(main) + '_' + message[subjectRow] + ".txt";
        MailFile << origpath << std::endl;
        mainIndex = 0;
    }
    MailFile.close();
    return mainIndex;
}

std::string readReference(std::string reference)
{

    std::string out;
    std::string contentLine;
    std::ifstream email;
    email.open(reference);
    while (1)
    {
        getline(email, contentLine);
        if (email.eof())
            break;
        out = out + contentLine + '\n';
    }
    email.close();
    return out;
}

void sendHandler(std::vector<std::string> message, char *dir, int soc, std::string username)
{
    std::string out = status_code[0];
    int count = 0;
    int subjectRow;
    int mainRow;
    //Check if multiple receivers.
    //Set up parameters
    if (message[1] == "y")
    {
        while (message[count + 2] != ".")
        {
            count++;
        }
        subjectRow = count + 3;
    }
    else if (message[1] == "n")
    {
        count = 1;
        subjectRow = 3;
    }
    else
    { //case input != (y/n)
        send(soc, (char *)status_code[1], strlen((char *)status_code[1]), 0);
        return;
    }
    //Main email
    if ((mainRow = saveEmail(username, message, subjectRow, "Pool", dir, soc, 0, false)) == -1)
        out = status_code[1];
    //Save references
    for (int i = 1; i < count + 1; i++)
    {
        if (saveEmail("Pool", message, subjectRow, message[i + 1], dir, soc, mainRow, false) == -1)
            out = status_code[1];
    }
    //Sent emails
    if (saveEmail(username, message, subjectRow, username, dir, soc, mainRow, true) == -1)
        out = status_code[1];

    //Procedure for sent emails
    send(soc, (char *)out.c_str(), strlen((char *)out.c_str()), 0);
}

void readHandler(std::vector<std::string> message, char *dir, int soc, std::string username)
{
    //Identify file
    std::string path = dir;
    path += '/' + username;
    DIR *dp;
    std::string out = "";
    if ((dp = opendir(path.c_str())))
    {
        struct dirent *ent;
        std::string contentLine;
        std::string firstline = "";
        //get subjects
        while ((ent = readdir(dp)) != NULL)
        {
            if (checkName(ent->d_name, message[1].c_str()) == 0)
            { //if it is the right file, go in
                std::ifstream content;
                content.open(path + '/' + ent->d_name);
                std::string origfile = "";
                while (1)
                {
                    getline(content, contentLine);
                    if (firstline == "")
                    {
                        firstline = contentLine;
                    }
                    else if (firstline == "***")
                    {
                        out = readReference(contentLine);
                        break;
                    }
                    out = out + contentLine;
                    if (content.eof())
                        break;
                    out = out + '\n';
                }
                content.close();
            }
        }
        closedir(dp);
    }

    if (out == "")
        out = "No emails with that number\n";
    send(soc, (char *)out.c_str(), strlen((char *)out.c_str()), 0);
}

void listHandler(std::vector<std::string> message, char *dir, int soc, std::string username)
{
    //define path
    std::string path = dir;
    path += '/' + username;
    int count = 0;
    DIR *dp;
    std::string out = "";

    if ((dp = opendir(path.c_str())))
    {
        struct dirent *ent;

        //get subjects
        while ((ent = readdir(dp)) != NULL)
        {
            std::string temp = ent->d_name;
            if (temp != "index" && temp != ".." && temp != ".")
            {
                count++;
                temp.replace(temp.end() - 4, temp.end(), "");
                out += temp + '\n';
            }
        }
        closedir(dp);
    }
    else
    {
        out = "No emails";
    }
    out.insert(0, std::to_string(count) + '\n');
    send(soc, (char *)out.c_str(), strlen((char *)out.c_str()), 0);
}

void deleteHandler(std::vector<std::string> message, char *dir, int soc, std::string username)
{
    //Identify file
    std::string path = dir;
    path += '/' + username;
    DIR *dp;

    // Returns Bad Request if file not found
    std::string out = status_code[3];

    if ((dp = opendir(path.c_str())))
    {
        struct dirent *ent;
        while ((ent = readdir(dp)) != NULL)
        {
            if (checkName(ent->d_name, message[1].c_str()) == 0)
            { //if it is the right file, go in
                std::string fileName = path + "/" + ent->d_name;
                if (remove(fileName.c_str()) == 0)
                {
                    out = status_code[0];
                    break;
                }
            }
        }
        closedir(dp);
    }
    send(soc, (char *)out.c_str(), strlen((char *)out.c_str()), 0);
}

void mailHandler(char *input, int clientSocket, char *directory, std::string username)
{
    std::vector<std::string> messageParsed;
    std::string parsed;
    std::string temp(input);
    std::stringstream strm(temp);
    while (std::getline(strm, parsed))
    {
        messageParsed.push_back(parsed);
    }

    if (messageParsed[0] == "SEND")
    {
        pthread_mutex_lock(&_mutex);
        sendHandler(messageParsed, directory, clientSocket, username);
        pthread_mutex_unlock(&_mutex);
    }
    else if (messageParsed[0] == "LIST")
    {
        pthread_mutex_lock(&_mutex);
        listHandler(messageParsed, directory, clientSocket, username);
        pthread_mutex_unlock(&_mutex);
    }
    else if (messageParsed[0] == "READ")
    {
        pthread_mutex_lock(&_mutex);
        readHandler(messageParsed, directory, clientSocket, username);
        pthread_mutex_unlock(&_mutex);
    }
    else if (messageParsed[0] == "DEL")
    {
        pthread_mutex_lock(&_mutex);
        deleteHandler(messageParsed, directory, clientSocket, username);
        pthread_mutex_unlock(&_mutex);
    }
    else
    {
        send(clientSocket, status_code[1], strlen(status_code[1]), 0);
    }
}
