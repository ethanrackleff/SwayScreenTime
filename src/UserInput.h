#ifndef USERINPUT_H
#define USERINPUT_H

#include <ncurses>
#include <iostream>

class UserInput {
    private:
     
    public:
        int lastCh;
        std::string lastStr;
        UserInput();
        //~UserInput();

        std::string getStr();
        char getCh();
}

#endif
