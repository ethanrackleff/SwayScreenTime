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
#include <filesystem>
#include <ncurses.h>
#include "UI.h"
#include "AppDataManager.h"
#include "SessionTracker.h"

using json = nlohmann::json;
using string = std::string;
using s_clock = std::chrono::steady_clock;

std::atomic<bool> runProgram(false);

/*
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

        //std::cout << "app_id = " << app_id << " ms elapsed = " << ms_Ellapsed.count() << "\n" << std::endl;
    }
};
*/SQLITE_ROW (100) - A row of data is available. Call sqlite3_column_*() functions to read the data, then call sqlite3_step() again for the next row.
SQLITE_DONE (101) - Statement completed successfully. No more rows (for SELECT) or the operation finished (for I
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

string run_Session(const string& cmd, string initApp, sqlite3* DB ) {
    Session sess;
    string currApp = initApp;
    string sql;
    sess.start_now(currApp);

     
    while (runProgram.load()) { //make a global flag here for when the program runs and stops.
        char buffer[8000];
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return "popen failed";
        }
        json json_buffer;
        char* msgErr;
        int exit = 0;

        while ((fgets(buffer,sizeof(buffer), pipe) != NULL) && runProgram.load()) {

            //std::cout << buffer << "\n" << std::endl;
            //std::cout << "New Buffer\n" << std::endl;

            json_buffer = json::parse(buffer);
            if (json_buffer["change"] == "focus") {
                sess.stop_now();
                
                sql = "INSERT INTO SESSION (APP, START, END, MSELAPSED) VALUES ('" 
                        + sess.app_id + "'," 
                        + std::to_string(sess.startCalendar) + ", " 
                        + std::to_string(sess.endCalendar) + ", " 
                        + std::to_string(sess.ms_Ellapsed.count()) + ");";
                exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &msgErr);
                if (exit != SQLITE_OK) {
                    //std::cout << "Error appending item\n" << std::endl;
                }
                else {
                    //std::cout << "Success appending item\n" << std::endl;
                }

                //std::cout << json_buffer["container"]["app_id"].template get<string>() << "\n" << std::endl;
                currApp = json_buffer["container"]["app_id"].template get<string>();            
                sess.start_now(currApp);
                }
        }
        pclose(pipe);
    }   
    sess.stop_now();
    return "";
}

/***make this function recursive eventually***/
string initial_focus(const json& root) {
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
    auto duration = std::chrono::seconds(30);

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

sqlite3* createTable() {
    sqlite3* DB;
    string create_table_sql = "CREATE TABLE SESSION("
                 "ID INTEGER PRIMARY KEY AUTOelectNCREMENT,"
                 "APP            TEXT        NOT NULL,"
                 "START          INTEGER   NOT NULL,"
                 "END            INTEGER   ," 
                 "MSELAPSED      INTEGER    );";
    int exit = 0;
    exit = sqlite3_open("example.db", &DB);
    char* msgErr;
    exit = sqlite3_exec(DB, create_table_sql.c_str(), NULL, 0, &msgErr);


    if (exit != SQLITE_OK) {
        //std::cout << "Error creating table" << std::endl;
        sqlite3_free(msgErr);
        return nullptr;
    }
    //std::cout << "Table Created" << std::endl;
    return DB;
}

int callbackPrintTotals(void* NotUsed, int argc, char** argv, char** azColName) {
    //argc = # of cols in row
    //argv[i] = column value as char*
    //azColName[i] = column name
    //gets executed for each row of sqlite3_exec

        std::string app = argv[0] ? argv[0] : "NULL";
        std::string ms_str = argv[1] ? argv[1] : "NULL";
        long long ms = std::stoi(ms_str);
        long long milliseconds = ms % 1000;
        long long seconds = ms / 1000;
        long long minutes = seconds / 60;
        seconds %= 60;
        long long hours = minutes / 60;
        minutes %= 60;
        long long days = hours / 24;
        hours %= 24;

                //std::cout << "App: " << app << " Time: " << days << "days, " << hours << "hrs, " << minutes << "min, " << seconds << "sec, " << milliseconds << "ms\n" << std::endl;

    return 0;
}

void printSQL(string sql, int (*callbackFunc)(void *, int, char **, char **), sqlite3* DB) {
    int exit = 0;
    char* msgErr = nullptr;
    exit = sqlite3_exec(DB, sql.c_str(), callbackFunc, 0, &msgErr);
    if (exit != SQLITE_OK) {
        //std::cout << "Error selecting item\n" << std::endl;
    }
    else {
        //std::cout << "Success selecting item\n" << std::endl;
    }
}

int callbackPrintToday(void* NotUsed, int argc, char** argv, char** azColName) {
        std::string app = argv[0] ? argv[0] : "NULL";
        std::string ms_str = argv[1] ? argv[1] : "0";
        long long ms = std::stoi(ms_str);
        long long milliseconds = ms % 1000;
        long long seconds = ms / 1000;
        long long minutes = seconds / 60;
        seconds %= 60;
        long long hours = minutes / 60;
        minutes %= 60;
        hours %= 24;
       
        //graph for time spent on app
        //std::cout << "V2 App:" << app; 
        for (int i = 0; i < 5; i++) {
            //std::cout << "#";
        }
        for (int i = 0; i < 5; i++) {
            //std::cout << " ";
        }

        //std::cout << " Time: " << hours << "hrs, " << minutes << "min, " << seconds << "sec, " << milliseconds << "ms\n" << std::endl;
        return 0;
}

int main(int argc, char** argv) {
    
    
    //make or open database
    sqlite3* DB;

    if (!(std::filesystem::exists("example.db"))) {
        DB = createTable();
        //std::cout << "In if in main";
    }
    else {
        sqlite3_open("example.db", &DB);
        //std::cout << "In else in main";
    }
    AppMonitor monitor(DB);
    //monitor.testDisplay();
    //monitor.testColors();
    //monitor.refreshWindows();
    nodelay(stdscr, FALSE);
    getch();
    //endwin();

    //Get currently focused window.
    /*
    string initOutput = run_read_cmd("swaymsg -t get_tree");
    json json_init_output = json::parse(initOutput);
    string initApp = initial_focus(json_init_output);
   
    //track screen time
    runProgram.store(true);
    std::thread killer {kill};
    run_Session("swaymsg -t subscribe -m '[\"window\"]'", initApp, DB );

    //stop tracking
    killer.join();

    printSQL("SELECT APP, SUM(MSELAPSED) AS TOTAL_MS FROM SESSION GROUP BY APP;", &callbackPrintTotals, DB);

    printSQL("SELECT APP, SUM(MSELAPSED) AS total_elapse FROM SESSION WHERE START >= strftime('%s', date('now', 'start of day')) AND END <= strftime('%s', date('now', 'start of day', '+1 day')) GROUP BY APP;", &callbackPrintToday, DB);
    */
    sqlite3_close(DB);
     

    return 0;
}
