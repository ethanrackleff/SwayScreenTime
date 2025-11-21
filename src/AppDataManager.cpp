#include "AppDataManager.h"
#include "SessionTracker.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <vector>
//*******************************************************************
//Creating and Storing Data******************************************
//*******************************************************************
AppDataManager::AppDataManager(const std::string& databasePath)
    : dbPath(databasePath) {
    int result = sqlite3_open(databasePath.c_str(), &database);
    if (result!= SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
    sqlite3_exec(database, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    createTables();
}
AppDataManager::~AppDataManager() {
    if (database) {
        sqlite3_close(database);
        database = nullptr;
    }
}
void AppDataManager::createTables() {
    //sqlite3* database;
    int result = 0;
    char* errMsg = nullptr;

    std::string createSessionTableSQL = 
        "CREATE TABLE IF NOT EXISTS SESSIONS("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "APP            TEXT        NOT NULL,"
        "START          INTEGER   NOT NULL,"
        "END            INTEGER   ," 
        "MSELAPSED      INTEGER    );";
    
    std::string createLimitTableSQL = 
        "CREATE TABLE IF NOT EXISTS LIMITS("
        "APP TEXT PRIMARY KEY,"
        "DAILY_LIMIT_MS INTEGER NOT NULL,"
        "ENABLED BOOLEAN NOT NULL DEFAULT 0);";

    //result = sqlite3_open(dbPath.c_str(), &database);
    result = sqlite3_exec(database, createSessionTableSQL.c_str(), nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to create SESSIONS table");
    }

    result = sqlite3_exec(database, createLimitTableSQL.c_str(), nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to create LIMITS table");
    }
}

void AppDataManager::saveSession(const SessionTracker::Session& session) {
    char* msgErr = nullptr;
    int result = 0;
    std::string insertSessionSQL = 
        "BEGIN TRANSACTION; "
        "INSERT INTO SESSIONS (APP, START, END, MSELAPSED) VALUES ('" 
        + session.app_id + "'," 
        + std::to_string(session.startCalendar) + ", " 
        + std::to_string(session.endCalendar) + ", " 
        + std::to_string(session.msElapsed.count()) + "); "
        "COMMIT;";

    result = sqlite3_exec(database, insertSessionSQL.c_str(), nullptr, nullptr, &msgErr);
    if (result != SQLITE_OK) {
        sqlite3_free(msgErr);
        throw std::runtime_error("Failed to save session to database");
    }
}

//*******************************************************************
//Blockers*****************************************************
//*******************************************************************
void AppDataManager::setAppLimit(std::string appName, long long limitMs) {
    char* msgErr = nullptr;
    int result = 0;
    std::string insertBlockSQL = 
        "INSERT INTO LIMITS (APP, DAILY_LIMIT_MS, ENABLED) VALUES ('test', 1000, 0);";
        /*"INSERT INTO LIMITS (APP, DAILY_LIMIT_MS, ENABLED) VALUES ('" 
        + appName + "', " 
        + std::to_string(limitMs) + ", " 
        + "0);"; //0 for false
        */

    result = sqlite3_exec(database, insertBlockSQL.c_str(), nullptr, nullptr, &msgErr);
    if (result != SQLITE_OK) {
        std::string error = "Failed to add block to LIMITS database: "+ std::string(sqlite3_errmsg(database));
        sqlite3_free(msgErr);
        throw std::runtime_error(error);
    } 
}
    //void setAppBlocking(const std::string& appName, bool enabled);
    //bool isAppBlocked(const std::string& appName);
long long AppDataManager::getTodaysUsageForApp(const std::string& appName) {
    std::string getTodaysUsageSQL = 
        "SELECT APP, SUM(MSELAPSED) AS dailyUsage "
        "FROM SESSIONS "
        "WHERE START >= strftime('%s', date('now', 'start of day')) "
        "AND END <= strftime('%s', date('now', 'start of day', '+1 day')) " 
        "AND APP == '" + appName + "' ";
        "GROUP BY APP;";
    long long dailyUsageMs = 0;
    sqlite3_stmt* stmt;

    int result = sqlite3_prepare_v2(database, getTodaysUsageSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getRemainingBlockTime query");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        dailyUsageMs = sqlite3_column_int64(stmt, 1);
    }
    sqlite3_finalize(stmt);
    return dailyUsageMs; 
}

long long AppDataManager::getLimitMsForApp(const std::string& appName) {
    std::string getLimitMsSQL = 
        "SELECT APP, DAILY_LIMIT_MS "
        "FROM LIMITS "
        "WHERE APP == '" + appName + "';";
    sqlite3_stmt* stmt;
    long long limitMs = 0;

    int result = sqlite3_prepare_v2(database, getLimitMsSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getLimitMsForApp query");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        long long limitMs = sqlite3_column_int64(stmt, 1);
    }
    sqlite3_finalize(stmt);

    return limitMs; 
}

bool AppDataManager::isAppBlocked(const std::string& appName) {
    std::string getBlockEnabledStatusSQL = 
        "SELECT APP, ENABLED "
        "FROM LIMITS "
        "WHERE APP == '" + appName + "';";
    sqlite3_stmt* stmt;
    bool enabled = false;

    int result = sqlite3_prepare_v2(database, getBlockEnabledStatusSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getBlockEnabledStatus query");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        enabled = sqlite3_column_int64(stmt, 1);
    }
    sqlite3_finalize(stmt);

    return enabled; 

}

float AppDataManager::calculateUsageLimitPercentage(const std::string& appName) {
    float usageLimitPercentage = 0;
    long long todaysUsageMs = getTodaysUsageForApp(appName);
    long long limitMs = getLimitMsForApp(appName);
    if (limitMs != (float)0) {
        usageLimitPercentage = (float)((float)todaysUsageMs / (float)limitMs);
    }
    return usageLimitPercentage;
}

std::vector<AppUsageData> AppDataManager::getBlocks() {
    std::vector<AppUsageData> blockData;
    std::string getBlockDataSQL = 
        "SELECT APP, DAILY_LIMIT_MS, ENABLED "
        "FROM LIMITS "
        "GROUP BY APP;";
    sqlite3_stmt* stmt;

    int result = sqlite3_prepare_v2(database, getBlockDataSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getBlocks query");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AppUsageData appData;
        
        const char* appName = (const char*)sqlite3_column_text(stmt, 0);
        long long dailyLimitMs = sqlite3_column_int64(stmt, 1);
        bool enabled = sqlite3_column_int64(stmt, 2);

        appData.appName = std::string(appName);
        appData.dailyUsageMs = 0;
        appData.totalUsageMs = 0;
        appData.dailyLimitMs = dailyLimitMs;
        appData.blockingEnabled = enabled;
        appData.currentSessionMs = 0;

        blockData.push_back(appData);
    }
    sqlite3_finalize(stmt);

    return blockData;
}

/*void AppDataManager::blockApp(AppUsageData app) {
    
}

void AppDataManager::checkBlocks() {

}*/

//*******************************************************************
//All Time Usage*****************************************************
//*******************************************************************
std::vector<AppUsageData> AppDataManager::getAllTimeUsage() {
    std::vector<AppUsageData> usageData;
    std::string getUsageByAppSQL = 
        "SELECT APP, SUM(MSELAPSED) AS totalUsage "
        "FROM SESSIONS "
        "GROUP BY APP;";
    sqlite3_stmt* stmt;

    int result = sqlite3_prepare_v2(database, getUsageByAppSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getAllTimeUsage query");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AppUsageData appData;
        
        const char* appName = (const char*)sqlite3_column_text(stmt, 0);
        long long totalUsageMs = sqlite3_column_int64(stmt, 1);

        appData.appName = std::string(appName);
        appData.dailyUsageMs = 0;
        appData.totalUsageMs = totalUsageMs;
        appData.dailyLimitMs = 0;
        appData.blockingEnabled =  false;
        appData.currentSessionMs = 0;

        usageData.push_back(appData);
    }
    sqlite3_finalize(stmt);

    return usageData;
}

//*******************************************************************
//Weekly Usage*******************************************************
//*******************************************************************
int AppDataManager::getCurrDayOfWeek() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    // tm_wday: 0=Sunday, 1=Monday, ..., 6=Saturday
    // Convert to Monday=0, Tuesday=1, ..., Sunday=6
    int mondayIndex = (tm.tm_wday + 6) % 7;
    return mondayIndex;
}

std::vector<std::map<std::string, long long>> AppDataManager::getThisWeeksUsage() {
    std::vector<std::map<std::string, long long>> weeklyUsageData;
    std::map<std::string, long long> dailyUsageData;
    std::string getUsageByAppSQL;
    sqlite3_stmt* stmt;
    int currDayOfWeek = getCurrDayOfWeek(); //0 is Monday
    
    //Get all days prior to today of this week app usage
    int result = 0;    
    int daysFromToday = 0;
    for (int i = 0; i < currDayOfWeek; i++) {
        dailyUsageData.clear();        
        daysFromToday = currDayOfWeek - i;

       getUsageByAppSQL = 
        "SELECT APP, SUM(MSELAPSED) AS weeklyUsage "
        "FROM SESSIONS "
        "WHERE START >= strftime('%s', date('now', 'start of day', '-" + std::to_string(daysFromToday) + " days')) "
        "AND END < strftime('%s', date('now', 'start of day', '-" + std::to_string(daysFromToday - 1) + " days')) "
        "GROUP BY APP;";

        result = sqlite3_prepare_v2(database, getUsageByAppSQL.c_str(), -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare getTodaysUsage query");
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            AppUsageData appData;
        
            const char* appName = (const char*)sqlite3_column_text(stmt, 0);
            long long dailyUsageMs = sqlite3_column_int64(stmt, 1);
            dailyUsageData[appName] = dailyUsageMs;
        }
        sqlite3_finalize(stmt);
        weeklyUsageData.push_back(dailyUsageData);
    }

    //Get current day App Usage
    getUsageByAppSQL =
        "SELECT APP, SUM(MSELAPSED) AS weeklyUsage "
        "FROM SESSIONS "
        "WHERE START >= strftime('%s', date('now', 'start of day')) "
        "AND END <= strftime('%s', date('now', 'start of day', '+1 day')) " 
        "GROUP BY APP;";

    result = sqlite3_prepare_v2(database, getUsageByAppSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getThisWeeksUsage query");
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* appName = (const char*)sqlite3_column_text(stmt, 0);
            long long dailyUsageMs = sqlite3_column_int64(stmt, 1);
            dailyUsageData[appName] = dailyUsageMs;
    }
    sqlite3_finalize(stmt);
    weeklyUsageData.push_back(dailyUsageData);

    return weeklyUsageData;
}

std::vector<long long> AppDataManager::getTotalUsageThisWeekByDay(std::vector<std::map<std::string, long long>> weeklyData) {
    std::vector<long long> totalWeeklyUsageByDay;
    long long todaysUsage = 0;
    for (int i = 0; i < static_cast<int>(weeklyData.size()); i++) {
        for (const auto& app : weeklyData[i]) {
            todaysUsage += app.second;
        }
        totalWeeklyUsageByDay.push_back(todaysUsage);
        todaysUsage = 0;
    }
    return totalWeeklyUsageByDay;
}

int AppDataManager::getMostScreenTimeDayThisWeek(std::vector<long long> totalWeeklyUsageByDay) {
    int maxMsIndex = 0;
    long long maxMs = 0;
    for (int i = 0; i < static_cast<int>(totalWeeklyUsageByDay.size()); i++) {
        //std::cout << "Day: " << i << " Usage: " << totalWeeklyUsageByDay[i] << std::endl;
        if (totalWeeklyUsageByDay[i] > maxMs) {
            maxMs = totalWeeklyUsageByDay[i];
            maxMsIndex = i;
        }
    }
    return maxMsIndex;
}

AppUsageData AppDataManager::getIthMostUsedAppThisWeek(int i) {
    AppUsageData appData;
    int currDayOfWeek = getCurrDayOfWeek();
    std::string getAppsByWeeklyUsageSQL =  
        "SELECT APP, SUM(MSELAPSED) AS weeklyUsage "
        "FROM SESSIONS "
        "WHERE START >= strftime('%s', date('now', 'start of day', '-" + std::to_string(currDayOfWeek) + " days')) "
        "AND END <= strftime('%s', date('now', 'start of day', '+1 day')) " 
        "GROUP BY APP "
        "ORDER BY weeklyUsage DESC "
        "LIMIT 1 OFFSET " + std::to_string(i - 1) + ";";
    sqlite3_stmt* stmt;
    
    int result = sqlite3_prepare_v2(database, getAppsByWeeklyUsageSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getIthMostUsedAppThisWeek query");
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        
        const char* appName = (const char*)sqlite3_column_text(stmt, 0);
        long long totalUsageMs = sqlite3_column_int64(stmt, 1);

        appData.appName = std::string(appName);
        appData.dailyUsageMs = 0;
        appData.totalUsageMs = totalUsageMs;
        appData.dailyLimitMs = 0;
        appData.blockingEnabled =  false;
        appData.currentSessionMs = 0;
    }
    sqlite3_finalize(stmt);

    return appData; 
}

std::vector<long long> AppDataManager::getThisWeeksUsageForApp(std::string appName) {
    std::vector<long long> weeklyUsageData;
    std::string getUsageByDaySQL;
    sqlite3_stmt* stmt;
    int currDayOfWeek = getCurrDayOfWeek(); //0 is Monday
    long long dailyUsageData = 0;
    long long dailyUsageMs = 0;
    //Get all days prior to today of this week app usage
    int result = 0;    
    int daysFromToday = 0;
    for (int i = 0; i < currDayOfWeek; i++) {
        daysFromToday = currDayOfWeek - i;
       getUsageByDaySQL = 
        "SELECT APP, SUM(MSELAPSED) AS weeklyUsage "
        "FROM SESSIONS "
        "WHERE START >= strftime('%s', date('now', 'start of day', '-" + std::to_string(daysFromToday) + " days')) "
        "AND END < strftime('%s', date('now', 'start of day', '-" + std::to_string(daysFromToday - 1) + " days')) "
        "AND APP == '" + appName + "';";

        result = sqlite3_prepare_v2(database, getUsageByDaySQL.c_str(), -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare getThisWeeksUsageForApp query");
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            dailyUsageMs = sqlite3_column_int64(stmt, 1);
            dailyUsageData += dailyUsageMs;
        }
        sqlite3_finalize(stmt);
        weeklyUsageData.push_back(dailyUsageData);
        dailyUsageData = 0;
    }

    //Get current day App Usage
    getUsageByDaySQL =
        "SELECT APP, SUM(MSELAPSED) AS weeklyUsage "
        "FROM SESSIONS "
        "WHERE START >= strftime('%s', date('now', 'start of day')) "
        "AND END <= strftime('%s', date('now', 'start of day', '+1 day')) " 
        "AND APP == '"+ appName +"';";

    result = sqlite3_prepare_v2(database, getUsageByDaySQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getThisWeeksUsageForApp query");
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
            dailyUsageMs = sqlite3_column_int64(stmt, 1);
            dailyUsageData += dailyUsageMs;
    }
    sqlite3_finalize(stmt);
    weeklyUsageData.push_back(dailyUsageData);

    return weeklyUsageData;
}

//*******************************************************************
//Daily Usage********************************************************
//*******************************************************************
long long AppDataManager::findMaximumUsageToday(std::vector<AppUsageData> todaysUsage) {
    long long maxUsageMs = 0;
    for (const auto& app : todaysUsage) {
        if (app.dailyUsageMs > maxUsageMs) {
            maxUsageMs = app.dailyUsageMs;
        }
    }
    return maxUsageMs;
}

std::vector<AppUsageData> AppDataManager::getTodaysUsage() {
    std::vector<AppUsageData> usageData;
    std::string getUsageByAppSQL = 
        "SELECT APP, SUM(MSELAPSED) AS dailyUsage "
        "FROM SESSIONS "
        "WHERE START >= strftime('%s', date('now', 'start of day')) "
        "AND END <= strftime('%s', date('now', 'start of day', '+1 day')) " 
        "GROUP BY APP;";
    sqlite3_stmt* stmt;

    int result = sqlite3_prepare_v2(database, getUsageByAppSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getTodaysUsage query");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AppUsageData appData;
        
        const char* appName = (const char*)sqlite3_column_text(stmt, 0);
        long long dailyUsageMs = sqlite3_column_int64(stmt, 1);

        appData.appName = std::string(appName);
        appData.dailyUsageMs = dailyUsageMs;
        appData.totalUsageMs = 0;
        appData.dailyLimitMs = 0;
        appData.blockingEnabled = false;
        appData.currentSessionMs = 0;

        usageData.push_back(appData);
    }
    sqlite3_finalize(stmt);

    return usageData;
}

long long AppDataManager::getTotalUsageToday() {
    std::string getTotalUsageSQL = 
        "SELECT SUM(MSELAPSED) as totalUsage "
        "FROM SESSIONS "
        "WHERE START >= strftime('%s', date('now', 'start of day')) "
        "AND END <= strftime('%s', date('now', 'start of day', '+1 day')) ";
    sqlite3_stmt* stmt;
    long long totalUsageMs = -1;

    int result = sqlite3_prepare_v2(database, getTotalUsageSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getTotalUsageToday query");
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
       totalUsageMs = sqlite3_column_int64(stmt, 0); 
       return totalUsageMs;
    }
    else {
        throw std::runtime_error("Failed to get totalUsageMs");
    }
}

std::map<std::string, long long> AppDataManager::getUsageTodayPercentage() {
    auto appData = getTodaysUsage(); 
    long long totalUsageTodayMs = getTotalUsageToday();
    std::map<std::string, long long> appToPercentMap;
    long long percent;
    if (totalUsageTodayMs != 0) {
        for (auto& app : appData) {
        percent = (static_cast<double>(app.dailyUsageMs) / totalUsageTodayMs) * 100;
        appToPercentMap[app.appName] = percent;
        }
    }
    else {
        throw std::runtime_error("Division by 0 in getUsageTodayPercentage");
    }
    
    return appToPercentMap;
}

void AppDataManager::setBlockEnabled(const std::string& appName, bool enabled) {
    std::string updateSQL = "UPDATE LIMITS SET ENABLED = " + std::to_string(enabled ? 1 : 0) + " WHERE APP = ?;";
    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(database, updateSQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare setBlockEnabled query");
    }
    sqlite3_bind_text(stmt, 1, appName.c_str(), -1, SQLITE_STATIC); //Fills ?
    result = sqlite3_step(stmt); //execute
    sqlite3_finalize(stmt); //clean up
    if (result != SQLITE_DONE) {
        throw std::runtime_error("Failed to update block enabled status");
    }
}

void AppDataManager::updateAppLimit(const std::string& appName, long long limitMs, bool enabled) {
    
    //Check if there is already a limit
    std::string checkSQL = "SELECT COUNT(*) FROM LIMITS WHERE APP = ?;";
    sqlite3_stmt* checkStmt;
    int result = sqlite3_prepare_v2(database, checkSQL.c_str(), -1, &checkStmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare updateAppLimit check query");
    }
    sqlite3_bind_text(checkStmt, 1, appName.c_str(), -1, SQLITE_STATIC);
    bool exists = false;
    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
        exists = (sqlite3_column_int(checkStmt, 0 > 0));
    }
    sqlite3_finalize(checkStmt);

    //Update if there is already a limit. Insert if there is not a limit
    std::string SQL;
    if (exists) {
        SQL = "UPDATE LIMITS SET DAILY_LIMIT_MS = ?, ENABLED = ? WHERE APP = ?;";
    }
    else {
        SQL = "INSERT INTO LIMITS (DAILY_LIMIT_MS, ENABLED, APP) VALUES (?, ?, ?);";
    }

    //Execute
    sqlite3_stmt* stmt;
    result = sqlite3_prepare_v2(database, SQL.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare updateAppLimit query");
    }
    sqlite3_bind_int64(stmt, 1, limitMs);
    sqlite3_bind_int(stmt, 2, enabled ? 1 : 0);
    sqlite3_bind_text(stmt, 3, appName.c_str(), -1, SQLITE_STATIC);
    result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (result != SQLITE_DONE) {
        throw std::runtime_error("Failed to update app limit");
    }
}


