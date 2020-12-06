
//CONSTANTS
#define PATHMAX 255
#define BUF 1024
#define BANSECONDS 3600
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
#include <ctime>
#include <csignal>
#include "src/mailHandler.cpp"
#include "src/myldap.cpp"

extern pthread_mutex_t _mutex;
// for cleanup:

struct banned
{
    struct in_addr address;
    std::time_t end;
};

std::vector<pthread_t> _threads;
std::vector<int> _sockets;
std::vector<struct banned> blacklist;

void *threadHandler(void *args);

struct threadArgs
{
    struct sockaddr_in cliaddress;
    int *newSocket;
    char *directory;
};

int usedSocket;
void sigHandler(int sig);

int main(int argc, char *argv[])
{

    //1 - INITALIZATION
    (void)signal(SIGINT, sigHandler);
    //Variables
    socklen_t addrlen;
    int socketPort, newSocket, isBanned, c;
    char directory[PATHMAX];
    struct sockaddr_in address, cliaddress;
    struct threadArgs *varthreadArgs = (struct threadArgs *)malloc(sizeof(struct threadArgs));
    char buffer[BUF];
    //Getopt section
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
        isBanned = 0;
        if (newSocket > 0)
        {
            _sockets.push_back(newSocket);

            std::vector<struct banned>::const_iterator it;
            for (it = blacklist.begin(); it != blacklist.end(); it++)
            {

                if (cliaddress.sin_addr.s_addr == it.base()->address.s_addr)
                {
                    // TODO: TIME does not work yet => check
                    std::cout << it.base()->end << " " << std::time(0) << std::endl;
                    // if (std::time(0) >= it.base()->end)
                    // {
                    std::cout << "User is banned.\n";
                    isBanned = 1;
                    break;
                    // }
                    // else
                    // {
                    //     blacklist.erase(it);
                    //     break;
                    // }
                }
            }
            if (isBanned == 1)
            {
                strcpy(buffer, "You are banned.\n");
                send(newSocket, buffer, strlen(buffer), 0);
                close(newSocket);
                continue;
            }
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
                    if (banCount > 2)
                    {
                        struct banned ban = {cliaddress.sin_addr, (std::time(0) + BANSECONDS)};
                        blacklist.push_back(ban);
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

    pthread_mutex_destroy(&_mutex);
    pthread_exit(NULL);
    return NULL;
}

void sigHandler(int sig)
{
    if (_threads.size() > 0)
    {
        for (auto &vec : _threads)
        {
            pthread_cancel(vec);
        }
    }
    if (_sockets.size() > 0)
    {
        for (auto &client : _sockets)
        {
            if (client > 0)
            {
                close(client);
            }
        }
    }
    close(usedSocket);
    pthread_exit(NULL);
    exit(sig);
}