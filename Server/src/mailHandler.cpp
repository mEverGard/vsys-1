#define SERVER_MAIL_HANDLER

#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>

void defaultMethod(std::vector<std::string> messageParsed)
{
    std::cout << "METHOD: " << messageParsed[0] << std::endl;
}

void mailHandler(char *input)
{
    // printf("MMessage received: %s", input);
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
        defaultMethod(messageParsed);
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
}
