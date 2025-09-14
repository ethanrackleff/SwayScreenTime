#include <nlohmann/json.hpp> // to parse JSON files. This is a dependency
#include <chrono> // to time how long a window has been focused
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <atomic> //for runProgram boolean

using json = nlohmann::json;
using string = std::string;
//bool runProgram = true;
std::atomic<bool> runProgram(false);

/*
struct timeMap {
    string app;
    int ms;
    int s;
    int m;
    int h;
    std::unordered_map map;

    void log(const& string app_id, int ms){ 
    
    }
}
*/

struct Session {

    using clock = std::chrono::steady_clock;
    string app_id;
    clock::time_point start{};
    clock::time_point end{};
    bool active = false;

    void start_now(string app) {
        app_id = app;
        start = clock::now();
        active = true;
    }

    int stop_now() {
        int output = 0;
        if (!active) {
            return 0;
        }
        end = clock::now();
        auto ms_Elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (end - start).count();
        std::cout << "app_id = " << app_id << " ms elapsed = " << ms_Elapsed << "\n";
        active = false;
        output = int(ms_Elapsed);
        std::cout << "In stop_now(). Here is output: " << output << "\n";
        return 0;
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

string run_Session(const string& cmd, string initApp) {
    Session sess;
    string currApp = initApp;
    bool current_runProgram = runProgram.load();
    while (current_runProgram) { //make a global flag here for when the program runs and stops.
        sess.start_now(currApp);
        char buffer[8000];
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return "popen failed";
        }
        json json_buffer;

        while ((fgets(buffer,sizeof(buffer), pipe) != NULL) && sess.active && current_runProgram) {
            current_runProgram = runProgram.load();
            
            std::cout << buffer << "\n";
            std::cout << "New Buffer\n";

            //maybe put this code into its own function to read swaymsg -t subscribe -m command output
            json_buffer = json::parse(buffer);
            if (json_buffer["change"] == "focus") {
                std::cout << json_buffer["container"]["app_id"].template get<string>() << "\n";
                currApp = json_buffer["container"]["app_id"].template get<string>();            
                sess.stop_now();
            }
        }
        pclose(pipe);
    }
    return "";
}


//make this function recursive eventually
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

int main() {
    //Get currently focused window.
    string initOutput = run_read_cmd("swaymsg -t get_tree");
    json json_init_output = json::parse(initOutput);
    string initApp = initial_focus(json_init_output);
    //std::cout << initApp;
    
    
    runProgram.store(true);
    run_Session("swaymsg -t subscribe -m '[\"window\"]'", initApp);

    //run program for length of duration
    auto start = std::chrono::steady_clock::now();
    auto duration = std::chrono::seconds(10);
    while (runProgram) {
        if (std::chrono::steady_clock::now() - start >= duration) {
            runProgram.store(false);
            std::cout << "The program should be stopping !" << std::endl;
            break;
        }
    }

    return 0;
}
