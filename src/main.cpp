#include <nlohmann/json.hpp> // to parse JSON files. This is a dependency
#include <chrono> // to time how long a window has been focused
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>

using json = nlohmann::json;

std::string run_read_cmd(const std::string& cmd) {
    char buffer[128]; //temporarily hold characters from command output
    std::string output = "";
    FILE* pipe = popen(cmd.c_str(), "r"); //convert cmd to a C string and read the output into pipe
    if (!pipe)
        return "popen failed";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        output += buffer;
    }
    pclose(pipe);
    return output;
}

//find node with "focused": true
/*
std::string initial_focus(const json& node) {
    if (node.is_object()) { //Base case of recursion is node not being a JSON file
        if (node.contains("name") && node["name"] == "root") {
            initial_focus(node["nodes"]); //Goes into root to look for different outputs
        }
        if (get_tree_output.contains("focused") && get_tree_output["focused"].is_boolean() && get_tree_output["focused"].template get<bool>() == true)  {
        return get_tree_output;
        }  

    }
    
    else {
        
    }
}
*/
int main() {
    std::string result = run_read_cmd("swaymsg -t get_tree");
    std::cout << result;
    return 0;
}
