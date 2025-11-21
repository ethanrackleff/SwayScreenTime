#include "SessionTracker.h"
#include "AppDataManager.h"
#include <ctime>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <thread>
#include <cstdio>
#include <sys/select.h>
#include <unistd.h>

SessionTracker::SessionTracker(AppDataManager* dataManager) 
    : dataManager(dataManager), isRunning(false) {
    if (!dataManager) {
        throw std::invalid_argument("Need a valid AppDataManager!");              
    }
}

SessionTracker::~SessionTracker() {
    stopTracking();
}
    
void SessionTracker::Session::start_now(const std::string& app) {
        app_id = app;
        startTime = std::chrono::steady_clock::now();
        active = true;
        time(&startCalendar);
    }

void SessionTracker::Session::stop_now() {
        if (!active) {
            return;
        }

        endTime = std::chrono::steady_clock::now();
        time(&endCalendar);
        msElapsed = std::chrono::duration_cast<std::chrono::milliseconds> (endTime - startTime);
        active = false;
        
    }

std::string SessionTracker::runReadCommand(const std::string& cmd) {
    
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

std::string SessionTracker::findInitialFocus(const nlohmann::json& root) {
    if (root["type"] == "root") {
//        std::cout << "In root\n";
       for (auto& output : root["nodes"]) {
//          std::cout << "In output\n";
           if (output["name"] != "__i3") {
                for (auto& workspace : output["nodes"]) {
                    if (workspace["type"] == "workspace") {
//                        std::cout << "In workspaces\n";
                        for (auto& window : workspace["nodes"]) {
//                            std::cout << "In windows\n";
                            if (window.contains("focused") 
                                && window["focused"].is_boolean() 
                                && window["focused"].template get<bool>() 
                                && window.contains("app_id") 
                                && !window["app_id"].is_null()) {
                                    return window["app_id"];
                            }
                            for (auto& window2 : window["nodes"]) {
                                if (window2.contains("focused") 
                                    && window2["focused"].is_boolean() 
                                    && window2["focused"].template get<bool>()
                                    && window.contains("app_id") 
                                    && !window["app_id"].is_null()) {
                                        return window2["app_id"];
                                }    
                            }
                        }
                    }
                }
            }
       }
    }
    return "Couldn't find focused window";
}

void SessionTracker::startTracking() {
    //Check for double-start
    if (isRunning.load()) {
        return;
    }

    isRunning.store(true);
    std::string getTreeOutput = runReadCommand("swaymsg -t get_tree");
    nlohmann::json jsonGetTreeOutput = nlohmann::json::parse(getTreeOutput);
    std::string currentApp = findInitialFocus(jsonGetTreeOutput);
    currentSession.start_now(currentApp);
    //monitorLoop();
    monitorThread = std::thread(&SessionTracker::monitorLoop, this);
}

void SessionTracker::stopTracking() {
    if (currentSession.active) {
        currentSession.stop_now();
        dataManager->saveSession(currentSession);
    }

    isRunning.store(false);
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
}

std::string SessionTracker::getCurrentApp() const {
    return currentSession.app_id;
}

bool SessionTracker::isActive() const {
    return currentSession.active;
}

void SessionTracker::monitorLoop() {
    const std::string cmd = "swaymsg -t subscribe -m '[\"window\"]'";
    FILE* pipe = nullptr;
    while (isRunning.load()) { 
        pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "Failed to start sway subscription" << std::endl;
            break;
        }

        char buffer[8000];
        while (isRunning.load()) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fileno(pipe), &fds);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; //100ms timeout

            int result = select(fileno(pipe) + 1, &fds, nullptr, nullptr, &timeout);
            
            if (result == 0) {
                //std::cerr << "Select timeout, isRunning=" << isRunning.load() << std::endl;
                break;
            }

            if (result > 0 && FD_ISSET(fileno(pipe), &fds)) {
                std::cerr << "Stuck in here" << std::endl;
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    try {
                        nlohmann::json json_buffer = nlohmann::json::parse(buffer);
                        if (json_buffer.contains("change")
                            && json_buffer["change"] == "focus") {
                                std::string newApp = "unknown";
                                if (json_buffer.contains("container") 
                                    && json_buffer["container"].contains("app_id") 
                                    && json_buffer["container"].contains("app_id")
                                    && !json_buffer["container"]["app_id"].is_null()) {
                                        currentSession.stop_now();
                                        dataManager->saveSession(currentSession);
                                        currentSession.start_now(newApp);
                                    }
                            }
                    } catch (const std::exception& e) {
                        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                    }
                } 
            } 
            else if (result < 0) {
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    }
    if (pipe) {
            system("pkill -f 'swaymsg -t subscribe'");
            pclose(pipe);
            pipe = nullptr;
    }
}
