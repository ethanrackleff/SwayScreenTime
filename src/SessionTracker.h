#ifndef SESSIONTRACKER_H
#define SESSIONTRACKER_H

#include <string>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>
#include <ctime>
#include <string>
#include <thread>

class AppDataManager;

class SessionTracker {
private:
    
    AppDataManager* dataManager;
    std::atomic<bool> isRunning;

    std::string runReadCommand(const std::string& cmd);
    std::string findInitialFocus(const nlohmann::json& root);

    void monitorLoop();

    std::thread monitorThread;

public:
    //A session represents an instance of time spent on app. We
    //We store the start and end time
    //We use functions to start a session and stop a session
    struct Session {
        std::string app_id;
        std::chrono::steady_clock::time_point startTime{};
        std::chrono::steady_clock::time_point endTime{};
        time_t startCalendar;
        time_t endCalendar;
        bool active = false;
        std::chrono::milliseconds msElapsed;

        void start_now(const std::string& app);
        void stop_now();
    };

    Session currentSession;

    SessionTracker(AppDataManager* dataManager);
    ~SessionTracker();
    
    //If application just started, we need to find the currently focused window and start a session with that. StartTracking() is the solution.
    void startTracking();
    void stopTracking();

    std::string getCurrentApp() const;
    bool isActive() const;
};
#endif
