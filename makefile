# Compiler and flags
CXX = g++
CXXFLAGS = -g -Wall -Iinc

# Executable names
ASSEMBLER = assembler
LINKER = linker
EMULATOR = emulator

# Source files
ASSEMBLER_SRCS = src/mainAssembler.cpp src/assembler.cpp src/symbolTable.cpp src/relocationTable.cpp
LINKER_SRCS = src/mainLinker.cpp src/linker.cpp
EMULATOR_SRCS = src/mainEmulator.cpp src/emulator.cpp

# Object files
ASSEMBLER_OBJS = $(ASSEMBLER_SRCS:.cpp=.o)
LINKER_OBJS = $(LINKER_SRCS:.cpp=.o)
EMULATOR_OBJS = $(EMULATOR_SRCS:.cpp=.o)

# Build targets
all: $(ASSEMBLER) $(LINKER) $(EMULATOR)

$(ASSEMBLER): $(ASSEMBLER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(LINKER): $(LINKER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(EMULATOR): $(EMULATOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Pattern rule for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(ASSEMBLER_OBJS) $(LINKER_OBJS) $(EMULATOR_OBJS) $(ASSEMBLER) $(LINKER) $(EMULATOR)

.PHONY: all clean
