#ifndef UI_H
#define UI_H
#include <ncurses.h>
#include <vector>
#include <string>
#include <map>
#include <sqlite3.h>
#include "AppDataManager.h"


class AppMonitor {
private:

    AppDataManager dataManager;
    // Window management
    WINDOW* graphWindow;
    WINDOW* blockWindow;
    WINDOW* statusWindow;
    WINDOW* mainWindow;

    // Screen dimensions
    int screenHeight, screenWidth;
    int graphHeight, graphWidth;
    int blockHeight, blockWidth;
    int statusHeight, statusWidth;

    // UI state
    int selectedAppIndex;
    int scrollOffset;
    bool editMode;
    std::string currentFocusedApp;
    long long maxTime;

    // Colors
    int colorPairNormal;
    int colorPairSelected;
    int colorPairWarning;
    int colorPairDanger;

    // Graph settings
    int graphBarWidth;
    int maxBarsInGraph;

public:
    AppMonitor(AppDataManager dataManager);
    ~AppMonitor();

    //Testing
    void testDisplay();
    void testColors();
    
    // Initialization and cleanup
    void initializeWindows();
    void initializeColors();
    void cleanup();

    void refreshWindows();


    // Data management
    void loadAppData();
    void refreshAppData();

    // Display functions
    void draw();
    void drawGraphWindow();
    void drawBlockWindow();
    void drawStatusWindow();
    void drawAppUsageGraph(const std::string& appName, int startY, int startX, int width, int height);

    // Input handling
    void handleInput();
    int processKeypress(int key);

    // App blocking interface
    void selectNextApp();
    void selectPreviousApp();
    void toggleAppBlocking();
    void editAppTimeLimit();
    void saveAppSettings();

    // Utility functions
    void resizeWindows();
    std::string formatTime(long long milliseconds);
    std::string formatTimeRemaining(long long usedMs, long long limitMs);
    void showMessage(const std::string& message);

    // Getters
    bool isAppBlocked(const std::string& appName);
    std::string getCurrentFocusedApp() const;
};

#endif
