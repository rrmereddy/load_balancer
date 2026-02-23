# complier and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Iinclude

# get the source files from the src directory
SRC = $(wildcard src/*.cpp)
OBJ = $(patsubst src/%.cpp, build/%.o, $(SRC))

# the executable file, will be used to run the load balancer
BIN = load_balancer

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# build the object files, put them in a build directory
# run the build with g++ and flags above
build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# build the balancer(if needed) then run it
run: clean $(BIN)
	./$(BIN)

# remove the build and bin
clean:
	rm -rf build $(BIN)
	rm -f load_balancer.txt
	rm -f load_balancer
