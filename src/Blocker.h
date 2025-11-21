#ifndef BLOCKER_H
#define BLOCKER_H

#include <string>
#include <vector>
#include "AppDataManager.h"
#include "SessionTracker.h"

class Blocker {
    
    private:
        AppDataManager* dataManager;
        SessionTracker* sessionTracker;
        //vector<AppUsageData> blockedData;

        std::vector<AppUsageData> getBlockedData();
        std::vector<AppUsageData> checkLimits();
        bool blockApp(const std::string& cmd);
        std::string executeCommand(const std::string& cmd);

    public:
        Blocker(AppDataManager* dataManager, SessionTracker* sessionTracker);
        ~Blocker();
        void checkAndBlock();
        
};

#endif
