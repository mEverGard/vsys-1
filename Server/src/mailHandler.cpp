#define SERVER_MAIL_HANDLER

#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>

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
    std::cout << "METHOD: " << messageParsed[0] << std::endl;
    // TODO: ROUTING HERE
    //     switch(messageParsed[0]) {
    //   case '':
    //     // code block
    //     break;
    //   case y:
    //     // code block
    //     break;
    //   default:
    //     // code block
}
}