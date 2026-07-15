CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

BIN := mini-shell
SRC := src/main.cpp

.PHONY: all clean run

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
