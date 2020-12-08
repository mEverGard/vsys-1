
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

std::string encryptEmail (std::string message, std::string key);
std::string decryptEmail (std::string message, std::string key);
std::string generateKey();
int calculateKeyValue(std::string key);
long moduloCalculator (long value, long modulo);


int calculateKeyValue(std::string key){
    int keyValue = 0;

    for (int i = 0; i < 5; i++){
        keyValue += int(key[i]);
    }
    keyValue = keyValue%100 + 1;
    return keyValue;
}

long moduloCalculator (long value, long modulo){

    if (value > 0) value = value % modulo;
    while (value < 0){
        value = value + modulo;
    }
    return value;
}

std::string encryptEmail(std::string message, std::string key){
    std::string encmsg = "";
    int keyValue = calculateKeyValue(key);
    for (int i = 0; i < message.length(); i++){
        //capital letters
        if (isupper(message[i])){
            encmsg += char(moduloCalculator(int(message[i] + keyValue - 65), 26) + 65);
        } else if (islower(message[i])){
            encmsg += char(moduloCalculator(int(message[i] + keyValue - 97), 26) + 97);
        } else {
            encmsg += message[i];
        }
    }
    //return encEmail;
    return encmsg;
}

std::string decryptEmail(std::string message, std::string key){

    if (key == "0") return message;
    std::string decmsg = "";
    int keyValue = calculateKeyValue(key);
    for (int i = 0; i < message.length(); i++){
        //capital letters
        if (isupper(message[i])){
            decmsg += char(moduloCalculator(int(message[i] - keyValue - 65), 26) + 65);
        //lower letters
        } else if (islower(message[i])){
            decmsg += char(moduloCalculator(int(message[i] - keyValue - 97), 26) + 97);
        } else {
            decmsg += message[i];
        }
    }
    //return encEmail;
    return decmsg;
}