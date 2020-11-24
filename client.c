
//CONSTANTS
#define BUF 1024

//IMPORTS
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int main(int argc, char* argv[]){
    
    //1 - INITIALIZATION

        //Variables
        char ipAddress[20];
        int socketPort;
        
        //Getopt section
        if (argc != 5){
            printf("Usage: %s -i IPAdress -p Port\n", argv[0]);
            return 1;
        }

        int c;      
        while ((c = getopt(argc, argv, "i:p:")) != EOF){
            switch (c){
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
        if ((create_socket = socket (AF_INET, SOCK_STREAM, 0)) == -1){
            perror("Socket error");
            return 1;
        }
    
        //Setting up the address
        memset(&address,0,sizeof(address));
        address.sin_family = AF_INET; //format ipV4
        address.sin_port = htons (socketPort); //port
        inet_pton(AF_INET, ipAddress, &address.sin_addr); //IP

        //Connection
        if (connect ( create_socket, (struct sockaddr *) &address, sizeof (address)) == 0){
            printf ("Connection with server (%s) established\n", inet_ntoa (address.sin_addr));
            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size > 0){
                buffer[size] = '\0';
                printf("%s", buffer);
            }
        } else {
                perror("Connect error - no server available");
                return 1;
        }

        //Send message
            do {
                printf ("Send message: ");
                fgets (buffer, BUF, stdin);
                send(create_socket, buffer, strlen (buffer), 0);
            }
            while (strcmp (buffer, "quit\n") != 0);
            close (create_socket);
            
        return 0;
}