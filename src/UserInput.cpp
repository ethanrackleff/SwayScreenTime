#include "UserInput.h"

#include <ncurses>
#include <iostream>

UserInput::UserInput() {
    int lastCh = 0;
    std::string lastStr = "";
    //currWindow
}

char UserInput::getCh(WINDOW& win) {
   initscr();
   cbreak();
   noecho();
   keypad(stdscr, TRUE);

   refresh();

   lastCh = wgetch(*win);

   refresh();
   endwin();
   return ch;
}

std::string getStr(WINDOW& win) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    refresh();

    char buffer[128];
    
    wgetStr(*win, buffer);
    lastStr = std::to_string(buffer);
    
    refresh();
    endwin();
    return lastStr;
}

