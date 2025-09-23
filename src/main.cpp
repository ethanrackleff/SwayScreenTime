#include <nlohmann/json.hpp> // to parse JSON files. This is a dependency
#include <chrono> // to time how long a window has been focused
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <atomic> //for runProgram boolean
#include <sqlite3.h>
#include <ctime>
#include <thread>
#include <string>

using json = nlohmann::json;
using string = std::string;
using s_clock = std::chrono::steady_clock;

std::atomic<bool> runProgram(false);

struct Session {

    string app_id;
    s_clock::time_point startTime{};
    s_clock::time_point endTime{};
    time_t startCalendar;
    time_t endCalendar;
    bool active = false;
    std::chrono::milliseconds ms_Ellapsed;

    void start_now(string app) {
        app_id = app;
        startTime = s_clock::now();
        active = true;
        time(&startCalendar);
    }

    void stop_now() {
        if (!active) {
            return;
        }

        endTime = s_clock::now();
        time(&endCalendar);
        ms_Ellapsed = std::chrono::duration_cast<std::chrono::milliseconds> (endTime - startTime);
        active = false;

        std::cout << "app_id = " << app_id << " ms elapsed = " << ms_Ellapsed.count() << "\n" << std::endl;
    }
};

string run_read_cmd(const string& cmd) {
    char buffer[128]; //temporarily hold characters from command output
    string output = "";
    FILE* pipe = popen(cmd.c_str(), "r"); //convert cmd to a C string and read the output into pipe
    if (!pipe)
        return "popen failed";
    while ((fgets(buffer, sizeof(buffer), pipe) != NULL) ){
        output += buffer;
    }
    pclose(pipe);
    return output;
}

string run_Session(const string& cmd, string initApp, sqlite3* DB, int id) {
    Session sess;
    string currApp = initApp;
    bool current_runProgram = runProgram.load();
    string sql;

     
    while (current_runProgram) { //make a global flag here for when the program runs and stops.
        sess.start_now(currApp);
        char buffer[8000];
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return "popen failed";
        }
        json json_buffer;
        char* msgErr;

        while ((fgets(buffer,sizeof(buffer), pipe) != NULL) && sess.active && current_runProgram) {
            current_runProgram = runProgram.load();
            
            std::cout << buffer << "\n" << std::endl;
            std::cout << "New Buffer\n" << std::endl;

            //maybe put this code into its own function to read swaymsg -t subscribe -m command output
            json_buffer = json::parse(buffer);
            if (json_buffer["change"] == "focus") {
                std::cout << json_buffer["container"]["app_id"].template get<string>() << "\n" << std::endl;
                currApp = json_buffer["container"]["app_id"].template get<string>();            
                sess.stop_now();
                
                sql = "INSERT INTO SESSION VALUES(" 
                        + std::to_string(id) + ", '" 
                        + sess.app_id + "', " 
                        + std::to_string(sess.startCalendar) + ", " 
                        + std::to_string(sess.endCalendar) + ", " 
                        + std::to_string(sess.ms_Ellapsed.count()) + ");";
                //sql = "INSERT INTO SESSION VALUES(" + std::to_string(id) +"\'" + std::to_string(sess.app_id) + "\', \'" + std::to_string(sess.startCalendar) + "\', \'" + std::to_string(sess.stopCalendar) + "\', " + std::to_string(sess.msElapsed) + ");" ;
                id++;
                sqlite3_exec(DB, sql.c_str(), NULL, 0, &msgErr);
            }
        }
        pclose(pipe);
    }
    return "";
}


/***make this function recursive eventually***/
//Finds focused window on startup
string initial_focus(const json& root) {
    if (root["type"] == "root") {
        std::cout << "In root\n";
       for (auto& output : root["nodes"]) {
           std::cout << "In output\n";
           if (output["name"] != "__i3") {
                for (auto& workspace : output["nodes"]) {
                    if (workspace["type"] == "workspace") {
                        std::cout << "In workspaces\n";
                        for (auto& window : workspace["nodes"]) {
                            std::cout << "In windows\n";
                            if (window.contains("focused") && window["focused"].is_boolean() && window["focused"].template get<bool>()) {
                                return window["app_id"];
                            }
                            for (auto& window2 : window["nodes"]) {
                                if (window2.contains("focused") && window2["focused"].is_boolean() && window2["focused"].template get<bool>()) {
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

void kill() {
    s_clock::time_point start = s_clock::now();
    s_clock::time_point currTime = s_clock::now();
    auto duration = std::chrono::seconds(10);

    while (runProgram.load()) {
        currTime = s_clock::now();
        if ((std::chrono::duration_cast<std::chrono::seconds> (currTime - start)) >= duration) {
            runProgram.store(false);
            std::cout << "The program should be stopping !" << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {

    sqlite3* DB;

    string sql = "CREATE TABLE SESSION("
                 "ID INT PRIMARY KEY         NOT NULL,"
                 "APP            TEXT        NOT NULL,"
                 "START          INTEGER   NOT NULL,"
                 "END            INTEGER   ," 
                 "MSELAPSED      INTEGER    );";
    int exit = 0;
    exit = sqlite3_open("example.db", &DB);
    char* msgErr;
    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &msgErr);

    if (exit != SQLITE_OK) {
        std::cout << "Error creating table" << std::endl;
        sqlite3_free(msgErr);
        return -1;
    }
    std::cout << "Table Created" << std::endl;

    //Get currently focused window.
    string initOutput = run_read_cmd("swaymsg -t get_tree");
    json json_init_output = json::parse(initOutput);
    string initApp = initial_focus(json_init_output);
    
    runProgram.store(true);
    std::thread killer {kill};
    run_Session("swaymsg -t subscribe -m '[\"window\"]'", initApp, DB, 0);
    killer.join();
    sqlite3_close(DB);
    

    return 0;
}
