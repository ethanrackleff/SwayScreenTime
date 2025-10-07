#include "UI.h"
#include "AppDataManager.h"
#include "SessionTracker.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {

    //CLI Version
    //sst -> sway screen time 
    //sst help 
    //cli(argc, argv);
    
     
    /*try {
        AppDataManager dataManager("example.db");
        std::cout << "Database initialized successfully" << std::endl;

        SessionTracker tracker(&dataManager);
        std::cout << "SessionTracker created successfully" << std::endl;

        tracker.startTracking();
        std::cout << "Session tracking started" << std::endl;
        
        //Use to run monitor screen time for 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(10));

        tracker.stopTracking();
        std::cout << "Session tracking stopped" << std::endl;

        auto todaysUsage = dataManager.getTodaysUsage();
        std::cout << "\n==== Today's Usage Data ====\n";
        for (const auto& app : todaysUsage) {
            std::cout << "App: " << app.appName 
                << ", Usage: " << app.dailyUsageMs << "ms" << std::endl;
        }

        auto allTimeUsage = dataManager.getAllTimeUsage();
        std::cout << "\n==== All Time Usage Data ====\n";
        for (const auto& app : allTimeUsage) {
            std::cout << "App: " << app.appName 
                << ", Usage: " << app.totalUsageMs << "ms" << std::endl;
        }

        long long totalMsToday = dataManager.getTotalUsageToday();
        std::cout << "\n==== Total Screen Time Today ====" << std::endl;
        std::cout << totalMsToday << "ms" << std::endl;

        auto todaysUsagePercentMap= dataManager.getUsageTodayPercentage();
        std::cout <<"\n=== Today's Usage Data Percentage ===\n";
        for (const auto& [app, percent] : todaysUsagePercentMap) {
            std::cout << "App: " << app
                << ", Usage: " << percent << "%" << std::endl;
        }
    } 
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl; 
        return 1;
    }
    */ 

    //TUI Version
    AppDataManager dataManager("example.db");
    std::cout << "Database initialized successfully" << std::endl;
    
     auto testUsage = dataManager.getTodaysUsage();
    std::cout << "Test: Found" << testUsage.size() << " apps in main()" << std::endl;

    AppMonitor monitor(&dataManager);
    monitor.draw();
    nodelay(stdscr, FALSE);
    getch();


    return 0;
}
