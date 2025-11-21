CXX = g++
CXXFLAGS = -std=c++17 -Wall
LIBS = -lsqlite3 -lncurses

TARGET = sway-screen-time
SOURCES = src/main.cpp src/SessionTracker.cpp src/AppDataManager.cpp src/UI.cpp src/Blocker.cpp src/UserInput.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

clean: 
	rm -f $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild
