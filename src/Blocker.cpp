#include "Blocker.h"
#include <iostream>
#include <cstdio>
#include <chrono>


Blocker::Blocker(AppDataManager* dataManager, SessionTracker* sessionTracker) 
    : dataManager(dataManager), sessionTracker(sessionTracker) {
    if (!dataManager) {
        throw std::invalid_argument("Blocker needs a valid AppDataManager.");
    }
    if (!sessionTracker) {
        throw std::invalid_argument("Blocker needs a valid SessionTracker");
    }
}

Blocker::~Blocker() {
    ;
}

std::string Blocker::executeCommand(const std::string& cmd) {
    
    char buffer[128];    
    std::string output = "";
    FILE* pipe = popen(cmd.c_str(), "r");     
    if (!pipe) {
        return "popen failed";
    }
    while ((fgets(buffer, sizeof(buffer), pipe) != NULL)){
        output += buffer;
    }
    pclose(pipe);
    return output;
}

std::vector<AppUsageData> Blocker::getBlockedData() {
    std::vector<AppUsageData> blockedData;

    try {
        std::vector<AppUsageData> allBlocks = dataManager->getBlocks();
        std::string currentApp = sessionTracker->getCurrentApp();

        for (auto& app : allBlocks) {
            if (app.blockingEnabled) {
                if (app.appName == currentApp && sessionTracker->isActive()) {
                    app.dailyUsageMs = dataManager->getTodaysUsageForApp(app.appName);
                    auto now = std::chrono::steady_clock::now();
                    auto sessionStart = sessionTracker->currentSession.startTime;
                    auto sessionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - sessionStart);
                }
                else {
                    app.dailyUsageMs = dataManager->getTodaysUsageForApp(app.appName);
                    app.currentSessionMs = 0;
                }
                blockedData.push_back(app);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "error getting blocked data: " << e.what() << std::endl;
    }

    return blockedData;
}

std::vector<AppUsageData> Blocker::checkLimits() {
    std::vector<AppUsageData> overLimitApps;

    std::vector<AppUsageData> blockedData = this->getBlockedData();

    for (const auto& app : blockedData) {
        if (app.dailyLimitMs > 0 && app.dailyUsageMs >= app.dailyLimitMs) {
            overLimitApps.push_back(app);
        }
    }

    return overLimitApps;
}

bool Blocker::blockApp(const std::string& appId) {
    if (appId.empty() || appId == "unknown") {
        return false;
    }

    std::string killCmd = "swaymsg '[app_id=\"" + appId + "\"]' kill";

    std::cout << "Blocing app: " << appId << std::endl;

    std::string result = executeCommand(killCmd);

    return (result != "popen failed");
}

void Blocker::checkAndBlock() {
    std::vector<AppUsageData> overLimitApps = checkLimits();

    std::string currentApp = sessionTracker->getCurrentApp();

    for (const auto& app : overLimitApps) {
        if (app.appName == currentApp) {
            blockApp(currentApp);
            break;
        }
    }
}




