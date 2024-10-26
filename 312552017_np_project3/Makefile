.PHONY: clean part1 part2

# Compiler
CXX = g++
CXXFLAGS = --std=c++20 -g -march=native -Wextra -Wpedantic -Wdisabled-optimization -Wredundant-decls -Wshadow
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))

# Llinking
UNIX_LIBS = -pthread -lboost_regex
WIN_LIBS = -lws2_32 -lwsock32 -lboost_regex

# Path
PROJECT_ROOT = . 
SERVER_PATH = ./cgiserver
UTIL_INC_PATH_FLAGS = -I./
PROJEST_PATH_FLAGS += -I$(PROJECT_ROOT)
SERVER_INC_PATH_FLAGS = -I$(SERVER_PATH)

# Source
HTTPSERVER_SRC = http_server.cpp
CONSOLE_SRC = console.cpp
CGISERVER_SRC = cgi_server.cpp
SERVER_SRC = $(wildcard $(SERVER_PATH)/*.cpp)

# Object
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
HTTPSERVER_OBJ = $(HTTPSERVER_SRC:.cpp=.o) $(SERVER_OBJ)
CGISERVER_OBJ = $(CGISERVER_SRC:.cpp=.o) $(SERVER_OBJ)
CONSOLE_OBJ = $(CONSOLE_SRC:.cpp=.o) $(SERVER_OBJ)

# # Program
# PROGRAM = part1 part2
# all: $(PROGRAM)

part1: http_server console.cgi
http_server: $(HTTPSERVER_OBJ)
	$(CXX) $(CPPFLAGS) $(CXX_INCLUDE_PARAMS) -o $@ $(HTTPSERVER_OBJ) $(UNIX_LIBS) $(CXX_LIB_PARAMS)
console.cgi: $(CONSOLE_OBJ)
	$(CXX) $(CPPFLAGS) $(CXX_INCLUDE_PARAMS) -o $@ $(CONSOLE_OBJ) $(UNIX_LIBS) $(CXX_LIB_PARAMS)

part2: cgi_server
cgi_server: $(CGISERVER_OBJ)
	$(CXX) $(CPPFLAGS) $(CXX_INCLUDE_PARAMS) -o $@ $(CGISERVER_OBJ) $(WIN_LIBS) $(CXX_LIB_PARAMS)


# part2: cgi_server.cpp
# 	g++ cgi_server.cpp -o cgi_server -lws2_32 -lwsock32 -std=c++14


# Dependency
$(HTTPSERVER_OBJ): CPPFLAGS = $(SERVER_INC_PATH_FLAGS)
$(CGISERVER_OBJ): CPPFLAGS = $(SERVER_INC_PATH_FLAGS)
$(CONSOLE_OBJ): CPPFLAGS = $(SERVER_INC_PATH_FLAGS)

clean:
	rm -f *.o $(SERVER_PATH)/*.o http_server console.cgi cgi_server
