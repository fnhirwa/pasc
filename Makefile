# Plain make build, equivalent to the CMake build.
CXX      ?= g++
CC       ?= cc
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Werror
CFLAGS   ?= -O1 -g -Wall -Wextra -Werror
BUILD    := build

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:src/%.cpp=$(BUILD)/%.o)

all: $(BUILD)/pasc $(BUILD)/libpasrt.a

$(BUILD)/pasc: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: src/%.cpp $(wildcard src/*.h) | $(BUILD)
	$(CXX) $(CXXFLAGS) -DPASC_RT_DEFAULT='"$(abspath $(BUILD))/libpasrt.a"' -c -o $@ $<

$(BUILD)/libpasrt.a: runtime/rt.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $(BUILD)/rt.o $<
	ar rcs $@ $(BUILD)/rt.o

$(BUILD):
	mkdir -p $(BUILD)

test: all
	tests/run.sh $(BUILD)/pasc

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
