.PHONY: clean debug npshell

CXX = g++
CXXFLAGS = --std=c++17 -g -flto -O3 -march=native -Wall -Wextra -Wpedantic -Wdisabled-optimization -Wredundant-decls -Wshadow

SHELL_PATH = ./shell
SHELL_INC_PATH_FLAGS = -I$(SHELL_PATH)
DEBUG_INC_PATH_FLAGS = -I./
SHELL_SRC = $(wildcard $(SHELL_PATH)/*.cpp)
NPSHELL_SRC = npshell.cpp

# Convert .cpp source files to .o object files
SHELL_OBJ = $(SHELL_SRC:.cpp=.o)
NPSHELL_OBJ = $(NPSHELL_SRC:.cpp=.o)

OBJ = $(NPSHELL_OBJ) $(SHELL_OBJ)

# Default flag
DEBUGFLAGS = -DNDEBUG

# Default build target
npshell: $(OBJ)
	$(CXX) $(CPPFLAGS) $(DEBUGFLAGS) -o $@ $(OBJ)

# Debug build target
debug: DEBUGFLAGS = -DDEBUG
debug: $(OBJ)
	$(CXX) $(CPPFLAGS) $(DEBUGFLAGS) -o npshell $(OBJ)

# Object files depend on these flags
$(OBJ): CPPFLAGS = $(SHELL_INC_PATH_FLAGS) $(CXXFLAGS) $(DEBUGFLAGS) $(DEBUG_INC_PATH_FLAGS)

# Clean target
clean:
	rm -f ./*.o $(SHELL_PATH)/*.o npshell
