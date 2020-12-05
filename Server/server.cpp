
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
#include <sys/stat.h>
#include "src/mailHandler.cpp"
#include "src/myldap.cpp"

extern pthread_mutex_t _mutex;
// for cleanup:
std::vector<pthread_t> _threads;
std::vector<int> _sockets;
std::vector<struct in_addr> blacklist;

void *threadHandler(void *args);

struct threadArgs
{
    struct sockaddr_in cliaddress;
    int *newSocket;
    char *directory;
};

int main(int argc, char *argv[])
{

    //1 - INITALIZATION

    //Variables
    socklen_t addrlen;
    int socketPort, usedSocket, newSocket;
    char directory[PATHMAX];
    struct sockaddr_in address, cliaddress;
    struct threadArgs *varthreadArgs = (struct threadArgs *)malloc(sizeof(struct threadArgs));

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
        // read/write/search permissions for owner and group, and with read/search permissions for others
        if (mkdir(directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        {
            perror("Failed to open/create directory.\n.");
            return 1;
        }
        printf("Directory not found => created\n");
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

    while (1)
    {
        printf("Waiting for connections...\n");
        newSocket = accept(usedSocket, (struct sockaddr *)&cliaddress, &addrlen);
        if (newSocket > 0)
        {

            _sockets.push_back(newSocket);

            // start thread

            varthreadArgs->cliaddress = cliaddress;
            varthreadArgs->newSocket = &newSocket;
            varthreadArgs->directory = directory;

            // https://www.thegeekstuff.com/2012/05/c-mutex-examples/
            pthread_t id;
            pthread_mutex_init(&_mutex, NULL);

            pthread_create(&id, NULL, threadHandler, (void *)varthreadArgs);
            // add signal handler to end thread
        }
    }

    //FINISH CONNECTION
    close(usedSocket);
    return 0;
}

void *threadHandler(void *args)
{
    char buffer[BUF];
    struct sockaddr_in cliaddress = ((struct threadArgs *)args)->cliaddress;
    int newSocket = *(((struct threadArgs *)args)->newSocket);
    char *directory = ((struct threadArgs *)args)->directory;
    int banCount = 0;
    printf("Incoming Connection from: %s:%d...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));
    strcpy(buffer, "Beep. Welcome to our unsecure Mailserver, please verify yourself.\n");
    send(newSocket, buffer, strlen(buffer), 0);

    int size;
    int loggedIn = 0;
    std::string username = "";
    while (true)
    {
        size = recv(newSocket, buffer, BUF - 1, 0);

        if (strncmp(buffer, "QUIT", 4) == 0)
        {
            break;
        }
        if (size > 0)
        {
            buffer[size] = '\0';
            if (loggedIn == 0)
            {
                // TODO: @JORGE do we need mutex here?
                pthread_mutex_lock(&_mutex);
                if ((username = ldapHandler(buffer, newSocket)) != "0")
                    loggedIn = 1;
                if (loggedIn == 0)
                {
                    banCount++;
                    std::cout << banCount << "text\n";
                    if (banCount > 2)
                    {
                        blacklist.push_back(cliaddress.sin_addr);
                        pthread_mutex_unlock(&_mutex);
                        // No return as we don't care if the user is banned.
                        break;
                    }
                }
                pthread_mutex_unlock(&_mutex);
            }
            else if (loggedIn == 1)
            {
                mailHandler(buffer, newSocket, directory, username);
            }
        }
        else if (size == 0)
        {
            printf("Client closed remote socket\n");
            break;
        }
        else
        {
            perror("recv error");
            break;
        }
    }
    close(newSocket);
    return NULL;
}