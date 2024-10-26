.PHONY: all clean

# Compiler settings
CXX = g++-9
CXXFLAGS = --std=c++17 -g -flto -O3 -march=native -Wall -Wextra -Wpedantic -Wdisabled-optimization -Wredundant-decls -Wshadow

# Define source files
SOURCES = noop.cpp number.cpp removetag0.cpp removetag.cpp

# Convert source file names to binary names (without extensions)
BINARIES = $(SOURCES:.cpp=)

# Target directory for binaries
BINDIR = ../bin

# Compile each source file directly into a binary
all: $(addprefix $(BINDIR)/,$(BINARIES))

$(BINDIR)/%: %.cpp
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) $< -o $@

clean:
	rm -rf $(BINDIR)
