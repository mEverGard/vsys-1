
//CONSTANTS
#define BUF 1024

//IMPORTS
#include <iostream>
#include <string>
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

using namespace std;

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

    //3 - MESSAGE SENDER
        string command;
        string message;
        //Message creator: message fits the entered command
        do {
            emailStart:
            printf ("Please enter the command: ");
            getline (cin, command);
            if (command == "SEND"){
                string sender, receiver, subject, body;
                getline(cin, sender);
                if (sender.length() > 8){
                    cout << "Username length is max. 8 characters. Start over.\n" << endl;
                    goto emailStart;
                }
                getline(cin, receiver);
                if (receiver.length() > 8){
                    cout << "Username length is max. 8 characters. Start over.\n" << endl;
                    goto emailStart;
                }
                getline(cin, subject);
                if (subject.length() > 80){
                    cout << "Subject length is max. 80 characters. Start over.\n" << endl;
                    goto emailStart;
                }
                string bodyline;
                while (bodyline != "."){
                    getline(cin, bodyline);
                    body = body + '\n' + bodyline;    
                }
                message = command + '\n' + sender + '\n' + receiver + '\n' + subject + '\n' + body + '\n'; //Append parts
            } else if (command == "READ" || command == "LIST" || command == "DEL") {
                string username;
                getline(cin, username);
                if (username.length() > 8){
                    cout << "Username length is max. 8 characters. Start over.\n" << endl;
                    goto emailStart;
                }
                message = command + '\n' + username + '\n';
                if (command == "DEL"){
                    string msgnumber;
                    getline(cin, msgnumber);
                    message = message + msgnumber + '\n';
                }
            } else {
                if (command != "QUIT") printf("Command does not exist\n");
            }
            //Message is sent
            char* msgsocket = &(message[0]); //Convert to C for using socket

            send(create_socket, msgsocket, strlen(msgsocket), 0); //Send through
        }
        while (command != "QUIT");
    
    //4 - CLOSE CONNECTION
    close (create_socket);
    return 0;
}