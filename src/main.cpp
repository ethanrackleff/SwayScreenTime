/*
To-do:
    - make initial_focus recursive
*/

#include <nlohmann/json.hpp> // to parse JSON files. This is a dependency
#include <chrono> // to time how long a window has been focused
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>

using json = nlohmann::json;
using string = std::string;

string run_read_cmd(const string& cmd) {
    char buffer[128]; //temporarily hold characters from command output
    string output = "";
    FILE* pipe = popen(cmd.c_str(), "r"); //convert cmd to a C string and read the output into pipe
    if (!pipe)
        return "popen failed";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        output += buffer;
    }
    pclose(pipe);
    return output;
}

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
    string result = run_read_cmd("swaymsg -t get_tree");
    std::cout << "This is the output: " << result;
    json get_tree_output = json::parse(result);
    string result2 = initial_focus(get_tree_output);
    std::cout << "this is the output: " << result2 << "\n";
    return 0;
}
