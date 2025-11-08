# Simple Makefile for Linux System Monitor

CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
LDFLAGS = -lncurses

SRC = src/main.cpp
BIN = bin/sysmon

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
