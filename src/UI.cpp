#include "UI.h"
#include "AppDataManager.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <iostream>

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
    statusHeight = 1;
    
    clear();
    //create windows
    graphWindow = newwin(graphHeight, graphWidth, 0, 0);
    blockWindow = newwin(blockHeight, blockWidth, 0, graphWidth);
    statusWindow = newwin(1, screenWidth, graphHeight, 0);
    mainWindow = stdscr;

    //enable scrolling
    scrollok(graphWindow, TRUE);
    scrollok(blockWindow, TRUE);

    //borders
    box(graphWindow, 0, 0);
    box(blockWindow, 0, 0);
    //box(statusWindow, 0, 0);
    
    //colors
    wbkgd(statusWindow, COLOR_PAIR(2));

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

//***********************************************************
//Tool Window**********************************************
//***********************************************************

//***********************************************************
//Status Window**********************************************
//***********************************************************

void AppMonitor::drawStatusWindow() {
    wbkgd(statusWindow, COLOR_PAIR(2));
}

//***********************************************************
//Blocker Window*********************************************
//***********************************************************

std::string AppMonitor::drawBlockGraph(AppUsageData& app, int width) {
    int barLength = 0;
    std::string barGraph = "[";
    if ((app.dailyLimitMs > 0) && (app.dailyLimitMs >= app.dailyUsageMs)) {
        barLength = static_cast<int>((static_cast<double>(app.dailyUsageMs) / app.dailyLimitMs) * (width - 2));
    }
    else {
        barLength = width - 2;
    }
    for (int i = 0; i < width - 2; i++) {
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

void AppMonitor::drawBlockWindow() {
    auto blockData = dataManager->getBlocks();
    int leftRightPadding = 2;
    int halfHeight = (graphHeight - 4) / 2;
    int maxBarHeight = halfHeight - 2;
    int yPosition = 2;
    int appNameWidth = 12;
    int limitWidth = 8;
    int enabledWidth = 8;
    //Fill middle of window with graph
    int graphWidthInternal = blockWidth - (leftRightPadding * 2) - appNameWidth - limitWidth - enabledWidth - 2;

//Headers************************************************************
    mvwprintw(blockWindow, yPosition, leftRightPadding, "%-*s %-*s %-*s %s", appNameWidth, "App Name", graphWidthInternal, "Graph", limitWidth, "Limit", "Enabled");

//Data***************************************************************
    std::string appName;
    std::string limit;
    std::string barGraph;
    std::string enabled;

    for (auto& app : blockData) {
        app.dailyUsageMs = dataManager->getTodaysUsageForApp(app.appName);
        appName = app.appName;
        limit = formatDailyTimeTruncated(app.dailyLimitMs);
        if (app.blockingEnabled) {
            enabled = "True";
        }
        else {
            enabled = "False";
        }
        barGraph = drawBlockGraph(app, graphWidthInternal);
        yPosition++;
       mvwprintw(blockWindow, yPosition, leftRightPadding, "%-*s %-*s %-*s %s", appNameWidth, appName.c_str(), graphWidthInternal, barGraph.c_str(), limitWidth, limit.c_str(), enabled.c_str()); 
    }
}

//***********************************************************
//Graph Window***********************************************
//***********************************************************

//Helper Functions for drawTodaysWindow()
std::string AppMonitor::formatDailyTimeTruncated(long long elapsedMs) {
    long long seconds;
    long long minutes;
    long long hours;
    std::string output = "";
 
    seconds = (elapsedMs / 1000);
    minutes = (seconds / 60) % 60;
    hours = (minutes / 60);
    seconds %= 60;
   
    if(hours != 0) {
        output = output + std::to_string(hours) + "h, ";
    }
    if (minutes != 0) {
            output = output + std::to_string(minutes) + "m, "; 
    }
        output = output + std::to_string(seconds) + "s";

    return output;
}
std::string AppMonitor::formatDailyTime(long long elapsedMs) {
    long long seconds;
    long long minutes;
    long long hours;
    std::string output = "";
 
    seconds = (elapsedMs / 1000);
    minutes = (seconds / 60) % 60;
    hours = (minutes / 60);
    seconds %= 60;

        output = std::to_string(hours) + "h, " + std::to_string(minutes) + "m, " + std::to_string(seconds) + "s";

    return output;
}

std::string createDailyBarGraph(long long dailyUsageMs, long long maxUsageMs, int graphWidthInternal) {
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

//Helper Function for drawWeeklyeWindow()

void AppMonitor::createWeeklyBar(int day, int xPosition) {


    auto weeklyUsage = dataManager->getThisWeeksUsage();
    auto currDay = dataManager->getCurrDayOfWeek();
    auto mostUsedApp = dataManager->getIthMostUsedAppThisWeek(1);
    auto secondMostUsedApp = dataManager->getIthMostUsedAppThisWeek(2);
    std::vector<long long> weeklyUsageByDayMostUsedApp = dataManager->getThisWeeksUsageForApp(mostUsedApp.appName);
    std::vector<long long> weeklyUsageByDaySecondMostUsedApp = dataManager->getThisWeeksUsageForApp(secondMostUsedApp.appName);
    std::vector<std::string> daysOfTheWeek = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    int spaceWidthForDays = graphWidth / 7; 
    int barWidth = graphWidth / 7 - 3;
    int halfHeight = (graphHeight - 4) / 2;
    int maxBarHeight = halfHeight - 2;
    std::vector<long long> totalWeeklyUsageByDay = dataManager->getTotalUsageThisWeekByDay(weeklyUsage);

    int highestScreenTimeDay = dataManager->getMostScreenTimeDayThisWeek(totalWeeklyUsageByDay);
    long long highestScreenTimeDayMs = totalWeeklyUsageByDay[highestScreenTimeDay];

    int currbarHeight = 0;
    int otherHeight = 0;
    float otherPercent = 0;
    int mostUsedHeight = 0;
    float mostUsedPercent = 0;
    int secondMostUsedHeight = 0;
    float secondMostUsedPercent = 0;
    
    

    std::string barString = "";
    if (totalWeeklyUsageByDay[day] != 0) {
        mostUsedPercent = (float)weeklyUsageByDayMostUsedApp[day] / (float)totalWeeklyUsageByDay[day];
        secondMostUsedPercent = (float)weeklyUsageByDaySecondMostUsedApp[day] / (float)totalWeeklyUsageByDay[day];
        otherPercent = (float)(1.0 - mostUsedPercent - secondMostUsedPercent);
    }
    if (highestScreenTimeDayMs != 0) {
        currbarHeight = (int)(((float)totalWeeklyUsageByDay[day] / highestScreenTimeDayMs) * (float)maxBarHeight);
    } 

    //std::cout << "mostUsedPercent: " << mostUsedPercent << std::endl;
    //std::cout << "secondMostUsedPercent: " << secondMostUsedPercent << std::endl;
    //std::cout << "otherPercent: " << otherPercent << std::endl;

    mostUsedHeight = (int)(currbarHeight * mostUsedPercent);
    secondMostUsedHeight = (int)(currbarHeight * secondMostUsedPercent);
    otherHeight = (int)(currbarHeight * otherPercent);
    //std::cout << "mostUsedHeight: " << mostUsedHeight << std::endl;
    //std::cout << "secondMostUsedHeight: " << secondMostUsedHeight << std::endl;
    //std::cout << "otherHeight: " << otherHeight << std::endl;


    int yPosition = graphHeight - 3;
    for (int h = 1; h <= mostUsedHeight; h++) {
        barString = "";
        for (int w = 1; w <= barWidth; w++) {
            barString = barString + "*";
        }
        mvwprintw(graphWindow, yPosition, xPosition, barString.c_str());
        yPosition--;
    }
    for (int h2 = 1; h2 <= secondMostUsedHeight; h2++) {
        barString = "";
        for (int w = 1; w <= barWidth; w++) {
            barString = barString + "@";
        }
        mvwprintw(graphWindow, yPosition, xPosition, barString.c_str());
        yPosition--;
    }
    for (int h3 = 1; h3 <= otherHeight; h3++) {
        barString = "";
        for (int w = 1; w <= barWidth; w++) {
            barString = barString + "#";
        }
        mvwprintw(graphWindow, yPosition, xPosition, barString.c_str());
        yPosition--;
    }
    std::string totalUsageTodayStr = formatDailyTimeTruncated(totalWeeklyUsageByDay[day]);
    mvwprintw(graphWindow, yPosition, xPosition, totalUsageTodayStr.c_str()); 
}

void AppMonitor::drawGraphWindow() {
//***********************************************************
//Todays Usage***********************************************************
//***********************************************************

//Variables***********************************************************
    auto todaysUsage = dataManager->getTodaysUsage();
    auto allTimeUsage = dataManager->getAllTimeUsage();
    
    int leftRightPadding = 2;
    int halfHeight = (graphHeight - 4) / 2;
    int maxBarHeight = halfHeight - 2;
    int yPosition = 1;
    int appNameWidth = 20;
    int usageWidth = 15;
    //Fill middle of window with graph
    int graphWidthInternal = graphWidth - (leftRightPadding * 2) - appNameWidth - usageWidth;

    //Ceiling on bar graphs
    long long maxUsageMs = dataManager->findMaximumUsageToday(todaysUsage);

//Headers************************************************************
    mvwprintw(graphWindow, yPosition, leftRightPadding, "Today: ");
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
        barGraph = createDailyBarGraph(todaysUsage[i].dailyUsageMs, maxUsageMs, graphWidthInternal);

        mvwprintw(graphWindow, yPosition, leftRightPadding, "%-*s %-*s %s", appNameWidth, appName.c_str(), graphWidthInternal, barGraph.c_str(), timeStr.c_str());
        yPosition++;
    }

//***********************************************************
//Weekly Usage***********************************************
//***********************************************************

///variables**************************************************************

    auto weeklyUsage = dataManager->getThisWeeksUsage();
    auto currDay = dataManager->getCurrDayOfWeek();
    auto mostUsedApp = dataManager->getIthMostUsedAppThisWeek(1);
    auto secondMostUsedApp = dataManager->getIthMostUsedAppThisWeek(2);
    std::vector<long long> weeklyUsageByDayMostUsedApp = dataManager->getThisWeeksUsageForApp(mostUsedApp.appName);
    std::vector<long long> weeklyUsageByDaySecondMostUsedApp = dataManager->getThisWeeksUsageForApp(secondMostUsedApp.appName);
    std::vector<std::string> daysOfTheWeek = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    int spaceWidthForDays = graphWidth / 7 - 2; 
    int barWidth = graphWidth / 7 - 4;

///headers**************************************************************
    yPosition = halfHeight + 3;
    mvwprintw(graphWindow, yPosition, leftRightPadding, "This Week: ");

    yPosition = graphHeight - 2;
    for (int i = 0; i < static_cast<int>(daysOfTheWeek.size()); i++)  {
        mvwprintw(graphWindow, yPosition, spaceWidthForDays * i + leftRightPadding,  daysOfTheWeek[i].c_str());
    }
    yPosition = maxBarHeight;
    std::string currBar = "";

///Weekly Graph**************************************************************
    for (int i = 0; i < static_cast<int>(weeklyUsage.size()); i++) {
        createWeeklyBar(i, spaceWidthForDays * i + leftRightPadding);
    }

///Key and Monthly Stats**************************************************************
///Implement monthly stats later
    yPosition = (graphHeight * 2) / 3;
    std::vector<std::string> keyString = {
        "Usage:",
       "* 1st: ",
        "  " + (mostUsedApp.appName.empty() ? "N/A" : truncateString(mostUsedApp.appName, 10)),
        "@ 2nd:",
        "  " + (secondMostUsedApp.appName.empty() ? "N/A" : truncateString(secondMostUsedApp.appName, 10)),
        "# Other"
    };
    for (std::string s : keyString) {
            mvwprintw(graphWindow, yPosition, spaceWidthForDays * 7 + leftRightPadding, s.c_str()); 
            yPosition++;
    }
    //yPosition -= 7;
    //mvwprintw(graphWindow, yPosition, spaceWidthForDays * 7 + leftRightPadding + 2, "Month Usage:"); 


    
}

//***********************************************************
//Constructors & Destructors*********************************
//***********************************************************

void AppMonitor::draw() {
    //werase(graphWindow);
    //werase(blockWindow);
    //werase(statusWindow);

    drawGraphWindow();
    drawBlockWindow();
    drawStatusWindow();

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

void AppMonitor::editAppTimeLimit() {
    std::vector<AppUsageData> blockData = dataManager->getBlocks();
    if (selectedAppIndex < 0 || selectedAppIndex >= static_cast<int>(blockData.size())) {
        return;
    }
    AppUsageData& app = blockData[selectedAppIndex];

    inputHandler.startEdit();

    while (inputHandler.isEditing()) {
        werase(blockWindow);
        box(blockWindow, 0, 0);
        mvwprintw(blockWindow, 0, 2, "Blockers");
        int yPos = 2 + selectedAppIndex + 1;
        mvwprintw(blockWindow, 2, 2, "Enter hours (press Enter to save, ESC to cancel): ");
        std::string buffer = inputHandler.getEditBuffer();
        mvwprintw(blockWindow, 3, 2, "Hours: %s_", buffer.c_str());
        wrefresh(blockWindow);
        int ch = getch();
        if (ch == 27) {//ESC
            inputHandler.cancelEdit();
            break;
        }
        bool done = inputHandler.handleEditKey(ch);
        if (done && inputHandler.isEditing() == false) {
            std::string input = inputHandler.getEditBuffer();
            if (!input.empty()) {
                try {
                    double hours = std::stod(input);
                    long long limitMs = static_cast<long long>(hours * 3600 * 1000);
                    dataManager->updateAppLimit(app.appName, limitMs, app.blockingEnabled);
                    wrefresh(statusWindow);
                    wrefresh(blockWindow);
                }
                catch (const std::exception e) {
                    mvwprintw(statusWindow, 0, 2, "Invalid input");
                    wrefresh(statusWindow);
                }
            }
            inputHandler.clearEditBuffer();
            break;
        }
    }
    draw();
}

void AppMonitor::handleInput(int ch) {
    InputAction action = inputHandler.processKey(ch);
    std::vector<AppUsageData> blockData = dataManager->getBlocks();
    
    switch(action) {
        case InputAction::NAVIGATE_UP:
            if (selectedAppIndex > 0) {
                selectedAppIndex--;
            }
            break;
        case InputAction::NAVIGATE_DOWN:
            if (selectedAppIndex < static_cast<int>(blockData.size()) - 1) {
                selectedAppIndex++;
            }
        case InputAction::TOGGLE:
            toggleAppBlocking();
            break;
        case InputAction::EDIT_LIMIT:
            editAppTimeLimit();
            break;
        default:
            break;
    }
}

void AppMonitor::toggleAppBlocking() {
      std::vector<AppUsageData> blockData = dataManager->getBlocks();

      if (selectedAppIndex >= 0 && selectedAppIndex < static_cast<int>(blockData.size())) {
          AppUsageData& app = blockData[selectedAppIndex];
          bool newStatus = !app.blockingEnabled;

          dataManager->setBlockEnabled(app.appName, newStatus);

          std::string msg = app.appName + ": Blocking " + (newStatus ? "ENABLED" : "DISABLED");
          mvwprintw(statusWindow, 0, 2, "%-*s", statusWidth - 4, msg.c_str());
          wrefresh(statusWindow);
      }
  }




