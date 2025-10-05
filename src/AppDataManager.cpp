#include "AppDataManager.h"
#include "SessionTracker.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <vector>

AppDataManager::AppDataManager(const std::string& databasePath)
    : dbPath(databasePath) {
    int result = sqlite3_open(databasePath.c_str(), &database);
    if (result!= SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    createTables();
}
AppDataManager::~AppDataManager() {
    if (database) {
        sqlite3_close(database);
        database = nullptr;
    }
}
void AppDataManager::createTables() {
    sqlite3* database;
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

    result = sqlite3_open(dbPath.c_str(), &database);
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
        "INSERT INTO SESSIONS (APP, START, END, MSELAPSED) VALUES ('" 
        + session.app_id + "'," 
        + std::to_string(session.startCalendar) + ", " 
        + std::to_string(session.endCalendar) + ", " 
        + std::to_string(session.msElapsed.count()) + ");";

    result = sqlite3_exec(database, insertSessionSQL.c_str(), nullptr, nullptr, &msgErr);
    if (result != SQLITE_OK) {
        sqlite3_free(msgErr);
        throw std::runtime_error("Failed to save session to database");
    }
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
        appData.currentSessionMs = 0;

        usageData.push_back(appData);
    }
    sqlite3_finalize(stmt);

    return usageData;
}



