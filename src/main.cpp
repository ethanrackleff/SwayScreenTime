#include "UI.h"
#include "AppDataManager.h"
#include "SessionTracker.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    try {
        AppDataManager dataManager("example.db");
        std::cout << "Database initialized successfully" << std::endl;

        SessionTracker tracker(&dataManager);
        std::cout << "SessionTracker created successfully" << std::endl;

        tracker.startTracking();
        std::cout << "Session tracking started" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(10));

        tracker.stopTracking();
        std::cout << "Session tracking stopped" << std::endl;

        auto todaysUsage = dataManager.getTodaysUsage();
        std::cout << "\n==== Today's Usage Data ====\n";
        for (const auto& app : todaysUsage) {
            std::cout << "App: " << app.appName 
                << ", Usage: " << app.dailyUsageMs << "ms" << std::endl;
        }
    } 
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl; 
        return 1;
    }
    return 0;
}
