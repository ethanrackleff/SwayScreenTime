#ifndef APPDATAMANAGER_H 
#define APPDATAMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>

struct AppUsageData {
    std::string appName;
    long long dailyUsageMs;
    long long dailyLimitMs;
    bool blockingEnabled;
    long long currentSessionMs;
};

class AppDataManager {
private:
    sqlite3* database;

    //Internal helpers
    void createTables();
    bool tableExists(const std::string& tableName);

public:
    //Constructor/Destructor
    AppDataManager(const std::string& databasePath);
    ~AppDataManager();

    //Session management
    void saveSession(const SessionTracker::Session& session);

    //App usage queries
    std::vector<AppUsageData> getTodaysUsage();
    std::vector<AppUsageData> getAllTimeUsage();
    AppUsageData getAppUsage(const std::string& appName);

    //App blocking/limits
    void setAppLimit(const std::string& appName, long long limitMs;
    void setAppBlocking(const std::string& appName, bool enabled);
    bool isAppBlocked(const std::string& appName);
    long long getRemainingTime(const std::string& appName);

    //Statistics
    long long getTotalUsageToday();
    std::string getMostUsedAppToday();
    std::map<std::string, long long> getUsageBreakdown();

    //Database maintenance
    void cleanOldSessions(int daysToKeep = 30);
    bool databaseHealth();
}

#endif
