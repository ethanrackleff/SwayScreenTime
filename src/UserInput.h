#ifndef USERINPUT_H
#define USERINPUT_H

#include <ncurses.h>
#include <iostream>

enum class InputAction {
    NONE,
    QUIT,
    NAVIGATE_UP,
    NAVIGATE_DOWN,
    TOGGLE,
    EDIT_LIMIT,
};

class UserInput {
    private:
        bool inEditMode;
        std::string editBuffer;

    public:
        UserInput();
        InputAction processKey(int ch);
        void startEdit();
        void cancelEdit();
        bool isEditing() const;
        bool handleEditKey(int ch);
        std::string getEditBuffer() const;
        void clearEditBuffer();
};

#endif
