#include "UserInput.h"

#include <ncurses.h>
#include <iostream>
#include <cctype> //for isdigit()

UserInput::UserInput()
    : inEditMode(false), editBuffer(""){
}

InputAction UserInput::processKey(int ch) {
    switch(ch) {
        case 'q':
        case 'Q':
            return InputAction::QUIT;
        case KEY_UP:
            return InputAction::NAVIGATE_UP;
        case KEY_DOWN:
            return InputAction::NAVIGATE_DOWN; 
        case ' ':
        case '\n':
        case KEY_ENTER:
            return InputAction::TOGGLE;
        case 'e':
        case 'E':
            return InputAction::EDIT_LIMIT;
        default:
            return InputAction::NONE;
    }
}

void UserInput::startEdit() {
    inEditMode = true;
    editBuffer = "";
}

void UserInput::cancelEdit() {
    inEditMode = false;
    editBuffer = "";
}

bool UserInput::isEditing() const {
    return inEditMode;
}

bool UserInput::handleEditKey(int ch) {
    switch(ch) {
        case '\n':
        case KEY_ENTER:
            inEditMode = false;
            return true;
        case 27: //ESC key
            cancelEdit();
            return true;
        case KEY_BACKSPACE:
        case 127: //backspace in some terminals
        case '\b': //backspace character
            if (!editBuffer.empty()) {
                editBuffer.pop_back();
            }
            break;
        default:
            if(isdigit(ch)) { //only accepts digits
                editBuffer += static_cast<char>(ch);
            }
            break;
    }
    return false; //still editing
}

std::string UserInput::getEditBuffer() const {
    return editBuffer;
}

void UserInput::clearEditBuffer() {
    editBuffer = "";
}