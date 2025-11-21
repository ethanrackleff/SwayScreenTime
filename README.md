# Sway Screen Time CLI
### Relevant Commands {#cmds}
* `swaymsg -t get_tree` to find the initial focused window.
* `swaymsg -t subscribe -m '["window"]'` says when the focused window has changed
Both of these commands output JSON text in the terminal. We will convert this JSON text to a JSON object and then parse it to find the focused window. The efficiency in this program will come from effective parsing.

### Functions
* [x] `std::string run_read_cmd(const std::string& cmd)` Runs a terminal command and returns the output of the command in a string. Used for getting the [relevant commands](#cmds) output.
* [x] `std::string initial_focus(const nlohmann::json& node)` Finds the window currently focused on start up.
* [ ] `std::string change_focus(const nlohmann::json& node)` Determines if a window has changed focus.
* [ ] `std::string initial_app_id(const nlohmann::json& node)` Gets application name on start uponce the initial focus window is found.
* [ ] `std::string change_app_id(const nlohman::json& node)` Gets application name if focused window updates.
* [ ] `change_session` Begins a new session to track the time in which an application is focused.

##### Structures
* [ ] Session structure

### To-Do
* [ ] Figure out how to store and source screen-time data efficiently and persistently. 
* [ ] Figure out how to implement `change_session`
* [ ] Figure out how the attributes of the `Session` structure

Testing
