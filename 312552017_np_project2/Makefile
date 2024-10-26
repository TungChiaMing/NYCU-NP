.PHONY: clean debug np_simple np_single_proc np_multi_proc

# Compiler
CXX = g++
CXXFLAGS = --std=c++17 -g -flto -O3 -march=native -Wextra -Wpedantic -Wdisabled-optimization -Wredundant-decls -Wshadow

# Definition
DEBUGFLAGS = -DNDEBUG
NPMULTIPROC_FLAGS = -DCONFIG_SHM

# Llinking
LIBS = -lrt

# Path
PROJECT_ROOT = . 
SHELL_PATH = ./shell
SERVER_PATH = ./server
DEBUG_INC_PATH_FLAGS = -I./
PROJEST_PATH_FLAGS += -I$(PROJECT_ROOT)
SHELL_INC_PATH_FLAGS = -I$(SHELL_PATH)
SERVER_INC_PATH_FLAGS = -I$(SERVER_PATH)

# Source
NPSIMPLE_SRC = np_simple.cpp
NPSINGLEPROC_SRC = np_single_proc.cpp
NPMULTIPROC_SRC = np_multi_proc.cpp
SHELL_SRC = $(wildcard $(SHELL_PATH)/*.cpp)
SERVER_SRC = $(wildcard $(SERVER_PATH)/*.cpp)

# Object
SHELL_OBJ = $(SHELL_SRC:.cpp=.o)
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
SERVER_OBJ_NPMULTIPROC = $(SERVER_SRC:.cpp=_npmultiproc.o)
NPSIMPLE_OBJ = $(NPSIMPLE_SRC:.cpp=.o) $(SHELL_OBJ) $(SERVER_OBJ)
NPSINGLEPROC_OBJ = $(NPSINGLEPROC_SRC:.cpp=.o) $(SHELL_OBJ) $(SERVER_OBJ)
NPMULTIPROC_OBJ = $(NPMULTIPROC_SRC:.cpp=.o) $(SHELL_OBJ) $(SERVER_OBJ_NPMULTIPROC)

# Program
PROGRAM = np_simple np_single_proc np_multi_proc
all: $(PROGRAM)

np_simple: $(NPSIMPLE_OBJ)
	$(CXX) $(CPPFLAGS) $(DEBUGFLAGS) -o $@ $(NPSIMPLE_OBJ)

np_single_proc: $(NPSINGLEPROC_OBJ)
	$(CXX) $(CPPFLAGS) $(DEBUGFLAGS) -o $@ $(NPSINGLEPROC_OBJ)

np_multi_proc: $(NPMULTIPROC_OBJ)
	$(CXX) $(CPPFLAGS) $(DEBUGFLAGS) -o $@ $(NPMULTIPROC_OBJ) $(LIBS)

# Dependency
$(NPSIMPLE_OBJ) $(NPSINGLEPROC_OBJ): CPPFLAGS = $(SHELL_INC_PATH_FLAGS) $(SERVER_INC_PATH_FLAGS) $(CXXFLAGS) $(DEBUGFLAGS) $(DEBUG_INC_PATH_FLAGS)
$(NPMULTIPROC_OBJ): CPPFLAGS = $(SHELL_INC_PATH_FLAGS) $(SERVER_INC_PATH_FLAGS) $(CXXFLAGS) $(DEBUGFLAGS) $(DEBUG_INC_PATH_FLAGS) $(NPMULTIPROC_FLAGS)
$(SERVER_PATH)/%_npmultiproc.o: $(SERVER_PATH)/%.cpp
	$(CXX) $(CXXFLAGS) $(PROJEST_PATH_FLAGS) $(SHELL_INC_PATH_FLAGS) $(NPMULTIPROC_FLAGS) -c $< -o $@

clean:
	rm -f ./*.o $(SHELL_PATH)/*.o $(SERVER_PATH)/*.o np_simple np_single_proc np_multi_proc
