
//CONSTANTS
#define PATHMAX 255
#define BUF 1024

//IMPORTS
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "src/mailHandler.cpp"

int main(int argc, char *argv[])
{

    //1 - INITALIZATION

    //Variables
    char buffer[BUF];
    socklen_t addrlen;
    int socketPort, usedSocket, newSocket;
    char directory[PATHMAX];
    struct sockaddr_in address, cliaddress;

    //Getopt section
    int c;
    if (argc != 5)
    {
        printf("Usage: %s -p port -d directory\n", argv[0]);
        return 1;
    }
    while ((c = getopt(argc, argv, "p:d:")) != EOF)
    {
        switch (c)
        {
        case 'p':
            //Validation pending
            socketPort = atoi(optarg);
            break;
        case 'd':
            //Validation pending
            strcpy(directory, optarg);
            break;
        case '?':
            printf("Not valid options");
            break;
        default:
            assert(0);
            break;
        }
    }

    //Checks if directory exist
    if (opendir(directory) == NULL)
    {
        perror("Failed to open directory. Start again\n.");
        return 1;
    }

    //2 - SOCKETING

    //create socket
    usedSocket = socket(AF_INET, SOCK_STREAM, 0);

    //bind socket to port
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;         //ipv4
    address.sin_addr.s_addr = INADDR_ANY; //own IP
    address.sin_port = htons(socketPort); //port in network order

    if (bind(usedSocket, (struct sockaddr *)&address, sizeof(address)) != 0)
    {
        perror("bind error");
        return 1;
    }
    //listen
    listen(usedSocket, 5);

    //3 - SERVER WORKING
    addrlen = sizeof(struct sockaddr_in);
    int size;

    while (1)
    {
        printf("Waiting for connections...\n");
        newSocket = accept(usedSocket, (struct sockaddr *)&cliaddress, &addrlen);
        if (newSocket > 0)
        {
            printf("Client connected from %s:%d...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));
            strcpy(buffer, "Welcome to myserver, Please enter your command:\n");
            send(newSocket, buffer, strlen(buffer), 0);
        }
        size = recv(newSocket, buffer, BUF - 1, 0);
        if (size > 0)
        {
            buffer[size] = '\0';
            mailHandler(buffer);
        }
        else if (size == 0)
        {
            printf("Client closed remote socket\n");
            break;
        }
        else
        {
            perror("recv error");
            return 1;
        }
        close(newSocket);
    }

    //FINISH CONNECTION
    close(usedSocket);
    return 0;
}