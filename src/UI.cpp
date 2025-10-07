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

//Helper Functions for drawTodaysUsage

std::string AppMonitor::formatDailyTime(long long elapsedMs) {
    long long seconds;
    long long minutes;
    long long hours;
    std::string output = "";
 
    seconds = (elapsedMs / 1000);
    minutes = (seconds / 60) % 60;
    hours = (minutes / 60);
    seconds %= 60000;
    
    output = std::to_string(hours) + "h, " + std::to_string(minutes) + "m, " + std::to_string(seconds) + "s";
    return output;
}

long long findMaximumUsage(std::vector<AppUsageData> todaysUsage) {
    long long maxUsageMs = 0;
    for (const auto& app : todaysUsage) {
        if (app.dailyUsageMs > maxUsageMs) {
            maxUsageMs = app.dailyUsageMs;
        }
    }
    return maxUsageMs;
}

std::string createBarGraph(long long dailyUsageMs, long long maxUsageMs, int graphWidthInternal) {
    int barLength = 0;
    std::string barGraph = "[";
    if (maxUsageMs > 0) {
        barLength = static_cast<int>((static_cast<double>(dailyUsageMs) / maxUsageMs) * graphWidthInternal);
    }
    for (int i = 0; i < graphWidthInternal - 2; i++) {
        if (i < barLength) {
           barGraph += "="; 
        }
        else {
            barGraph += " ";
        }
    }
    barGraph += "]";
    return barGraph;
}

std::string truncateString(std::string appName, int appNameWidth) {
    if ((static_cast<int>(appName.length())) > appNameWidth) {
        appName = appName.substr(0, appNameWidth - 3) + "...";
    }
    return appName;
}

void AppMonitor::drawGraphWindow() {
//Variables***********************************************************
    auto todaysUsage = dataManager->getTodaysUsage();
    auto allTimeUsage = dataManager->getAllTimeUsage();
    
    int leftRightPadding = 2;
    int halfHeight = (graphHeight - 4) / 2;
    int yPosition = 1;
    int appNameWidth = 20;
    int usageWidth = 15;
    //Fill middle of window with graph
    int graphWidthInternal = graphWidth - (leftRightPadding * 2) - appNameWidth - usageWidth;

    //Ceiling on bar graphs
    long long maxUsageMs = findMaximumUsage(todaysUsage);
    
//Headers************************************************************
    mvwprintw(graphWindow, yPosition, leftRightPadding, "Today's Usage: ");
    yPosition += 2;
    mvwprintw(graphWindow, yPosition, leftRightPadding, "%-*s %-*s %s", appNameWidth, "App Name", graphWidthInternal, "Graph", "Usage");
    yPosition += 1;

///Data**************************************************************
    std::string appName;
    std::string timeStr;
    std::string barGraph;

    for(size_t i = 0; i < todaysUsage.size() && yPosition < halfHeight + 1; ++i) {
        appName = truncateString(todaysUsage[i].appName, appNameWidth);
        timeStr = formatDailyTime(todaysUsage[i].dailyUsageMs);
        barGraph = createBarGraph(todaysUsage[i].dailyUsageMs, maxUsageMs, graphWidthInternal);

        mvwprintw(graphWindow, yPosition, leftRightPadding, "%-*s %-*s %s", appNameWidth, appName.c_str(), graphWidthInternal, barGraph.c_str(), timeStr.c_str());
        yPosition++;
    }
}

void AppMonitor::draw() {
    //werase(graphWindow);
    //werase(blockWindow);
    //werase(statusWindow);

    drawGraphWindow();
    //drawBlockWindow();
    //drawStatusWindow();

    refreshWindows();
}

AppMonitor::AppMonitor(AppDataManager* dataManager) : dataManager(dataManager), selectedAppIndex(0), scrollOffset(0), editMode(false) {
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





