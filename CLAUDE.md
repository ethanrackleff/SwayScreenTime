# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Sway Screen Time TUI is a C++ terminal application that tracks window focus time in the Sway window manager and provides application blocking capabilities. It monitors which applications are focused, stores usage statistics in a SQLite database, and can automatically close applications when time limits are exceeded. The application features a full ncurses-based TUI for monitoring usage and configuring app blocking rules.

## Architecture

The application consists of:

- **main.cpp**: Core application with session tracking, Sway IPC integration, and database operations
- **UI.cpp**: ncurses-based TUI components including the AppMonitor class for real-time usage display and app blocking interface
- **example.db**: SQLite database storing session data with APP, START, END, and MSELAPSED columns

### Key Components

- **Session struct**: Tracks application focus sessions with start/end times and elapsed duration
- **AppMonitor class**: ncurses TUI manager with separate windows for graphs, blocking controls, and status display
- **initial_focus()**: Recursively parses Sway's JSON tree structure to find the initially focused window
- **run_Session()**: Main event loop that subscribes to Sway window events and tracks focus changes
- **App blocking system**: Monitors usage time and uses `swaymsg [criteria] kill` to close applications exceeding time limits
- **Sway IPC integration**: Uses `swaymsg -t get_tree`, `swaymsg -t subscribe -m '["window"]'`, and `swaymsg [criteria] kill` commands

## Dependencies

The project requires:
- nlohmann/json library for JSON parsing
- SQLite3 for data persistence and app blocking configuration
- ncurses for full-featured TUI (graphs, menus, real-time displays)
- C++17 standard library features (chrono, atomic, thread, filesystem)

## Build Commands

To compile the application:
```bash
g++ -std=c++17 -o sway-screen-time main.cpp -lsqlite3 -lncurses
```

## Running the Application

The compiled binary `sway-screen-time` will:
1. Launch a full-screen ncurses TUI with multiple panels
2. Create or open the SQLite database (`example.db`)
3. Display real-time usage statistics with visual graphs
4. Provide interface for configuring app time limits and blocking rules
5. Continuously monitor applications and enforce time limits by closing apps when exceeded
6. Show current session data, daily totals, and app blocking status

## Database Schema

**SESSION table** (usage tracking):
- ID: Primary key (auto-increment)
- APP: Application identifier (TEXT)
- START: Start timestamp (INTEGER, Unix time)
- END: End timestamp (INTEGER, Unix time)
- MSELAPSED: Duration in milliseconds (INTEGER)

**APP_LIMITS table** (blocking configuration):
- APP: Application identifier (TEXT, primary key)
- DAILY_LIMIT_MS: Daily time limit in milliseconds (INTEGER)
- ENABLED: Whether blocking is active for this app (BOOLEAN)

## Sway Integration

The application integrates with Sway compositor through IPC:
- **Window monitoring**: `swaymsg -t get_tree` for initial state and `swaymsg -t subscribe -m '["window"]'` for focus events
- **App identification**: Parses JSON responses to extract app_id from focused windows
- **App blocking**: Uses `swaymsg '[app_id="<app_name>"] kill'` to close applications when time limits exceeded
- **Window criteria**: Supports various Sway criteria for targeting specific windows/applications

## TUI Features

The ncurses interface provides:
- **Real-time usage graphs**: Visual representation of daily app usage
- **App blocking controls**: Set time limits and enable/disable blocking per application
- **Live statistics**: Current session time, daily totals, and remaining time before blocking
- **Multi-panel layout**: Separate windows for graphs, controls, and status information
- **Interactive navigation**: Keyboard controls for managing app limits and viewing detailed statistics