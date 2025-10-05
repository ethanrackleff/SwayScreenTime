#include "UI.h"
#include "AppDataManager.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <chrono>

void AppMonitor::refreshWindows() {
    wrefresh(mainWindow);
    wrefresh(graphWindow);
    wrefresh(blockWindow);
    wrefresh(statusWindow);
}

void AppMonitor::initializeWindows() {
    //initial settings
    initscr();
    keypad(stdscr, TRUE); //to detect arrow keys
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0); //no cursor
    
    //set screen dimensions
    getmaxyx(stdscr, screenHeight, screenWidth); //gets num of rows and cols and places in screenHeight and screenWidth    
    graphWidth = int(screenWidth * 0.6); //60% of screen width
    graphHeight = screenHeight - 4; //4 lines for status
    blockWidth = screenWidth - graphWidth;
    blockHeight = graphHeight;
    statusWidth = screenWidth;
    statusHeight = 4;

    //create windows
    graphWindow = newwin(graphHeight, graphWidth, 0, 0);
    blockWindow = newwin(blockHeight, blockWidth, 0, graphWidth);
    statusWindow = newwin(statusHeight, statusWidth, graphHeight, 0);
    mainWindow = stdscr;

    //enable scrolling
    scrollok(graphWindow, TRUE);
    scrollok(blockWindow, TRUE);

    //borders
    box(graphWindow, 0, 0);
    box(blockWindow, 0, 0);
    box(statusWindow, 0, 0);

    //titles
    mvwprintw(graphWindow, 0, 2, "App Usage");
    mvwprintw(blockWindow, 0, 2, "Blockers");
    mvwprintw(statusWindow, 0, 2, "Status");
 
    //display content
    refreshWindows(); 
    //mvwprintw(mainWindow, 0, 0, "INIT CALLED");

}

void AppMonitor::testDisplay() {
    mvwprintw(graphWindow, 2, 2, "Graph Window working");
    mvwprintw(blockWindow, 2, 2, "Block Window working");
    mvwprintw(statusWindow, 2, 2, "Status window working");
    
    initializeWindows();
}

void AppMonitor::initializeColors() {
   if (!has_colors()) {
       mvwprintw(graphWindow, 0, 0, "No color support");
        return; //no colors supported in this terminal.
    }
    
   start_color();

   init_pair(1, COLOR_WHITE, COLOR_BLACK);
   init_pair(2, COLOR_BLACK, COLOR_WHITE);
   init_pair(3, COLOR_YELLOW, COLOR_BLACK);
   init_pair(4, COLOR_RED, COLOR_BLACK);

   colorPairNormal = 1;
   colorPairSelected = 2;
   colorPairWarning = 3;
   colorPairDanger = 4;
}

void AppMonitor::testColors() {
    wattron(graphWindow, COLOR_PAIR(colorPairNormal));
    wattron(blockWindow, COLOR_PAIR(colorPairSelected));
    wattron(statusWindow, COLOR_PAIR(colorPairWarning));
    mvwprintw(statusWindow, 0, 0, "testColors working");
    refreshWindows();
}

void AppMonitor::draw() {
    clear();

    drawGraphWindow();
    drawBlockWindow();
    drawStatusWindow();

    refreshWindows();
}

AppMonitor::AppMonitor(AppDataManager dataManager) : dataManager(dataManager), selectedAppIndex(0), scrollOffset(0), editMode(false) {
    maxTime = 0;
    currentFocusedApp = "";

    initializeWindows();
    initializeColors();
//loadAppData(); 
}

AppMonitor::~AppMonitor() {
    if (graphWindow) { 
        delwin(graphWindow);
    }
    if (blockWindow) {
        delwin(blockWindow);
    }
    if (statusWindow) {
        delwin(statusWindow);
    }
    endwin();
}





