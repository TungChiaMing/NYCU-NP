.PHONY: clean

# Compiler
CXX = g++
CXXFLAGS = --std=c++20 -g -march=native -Wextra -Wpedantic -Wdisabled-optimization -Wredundant-decls -Wshadow

# Llinking
UNIX_LIBS = -pthread -lboost_regex

# Path
PROJECT_ROOT = . 
CGI_SERVER_PATH = ./cgiserver
SOCKS4_SERVER_PATH = ./socks4server

UTIL_INC_PATH_FLAGS = -I./
PROJEST_PATH_FLAGS += -I$(PROJECT_ROOT)
CGI_SERVER_INC_PATH_FLAGS = -I$(CGI_SERVER_PATH)
SOCKS4_SERVER_INC_PATH_FLAGS = -I$(SOCKS4_SERVER_PATH)

# Source
SOCKS4_SERVER_SRC = socks_server.cpp
CONSOLE_SRC = console.cpp

CGI_SERVER_SRC = $(wildcard $(CGI_SERVER_PATH)/*.cpp)
SOCKS4_HANDLER_SRC = $(wildcard $(SOCKS4_SERVER_PATH)/*.cpp)

# Object
CGI_SERVER_OBJ = $(CGI_SERVER_SRC:.cpp=.o)
SOCKS4_HANDLER_OBJ = $(SOCKS4_HANDLER_SRC:.cpp=.o)

CONSOLE_OBJ = $(CONSOLE_SRC:.cpp=.o) $(CGI_SERVER_OBJ)
SOCKS4_SERVER_OBJ = $(SOCKS4_SERVER_SRC:.cpp=.o) $(SOCKS4_HANDLER_OBJ)

# Program
PROGRAM = socks_server hw4.cgi
all: $(PROGRAM)

socks_server: $(SOCKS4_SERVER_OBJ)
	$(CXX) $(CPPFLAGS) -o $@ $(SOCKS4_SERVER_OBJ) $(UNIX_LIBS)

hw4.cgi: $(CONSOLE_OBJ)
	$(CXX) $(CPPFLAGS) -o $@ $(CONSOLE_OBJ) $(UNIX_LIBS)


# Dependency
$(SOCKS4_SERVER_OBJ): CPPFLAGS = $(SOCKS4_SERVER_INC_PATH_FLAGS)
$(CONSOLE_OBJ): CPPFLAGS = $(PROJEST_PATH_FLAGS) $(CGI_SERVER_INC_PATH_FLAGS) $(SOCKS4_SERVER_INC_PATH_FLAGS)

clean:
	rm -f *.o $(CGI_SERVER_PATH)/*.o $(SOCKS4_SERVER_PATH)/*.o socks_server hw4.cgi
