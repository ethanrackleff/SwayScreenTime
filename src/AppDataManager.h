#include <string>

struct AppUsageData {
    std::string appName;
    long long dailyUsageMs;
    long long dailyLimitMs;
    bool blockingEnabled;
    long long currentSessionMs;
};

