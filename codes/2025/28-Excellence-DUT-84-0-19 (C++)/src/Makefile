target=compiler
CC=g++
CXXFLAGS += -std=c++17  -g -w
LIBS += -lantlr4-runtime

# Source files
SRCS = $(wildcard *.cpp)

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(target)

$(target): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS) 

%.o: %.cpp
	$(CC) -c $< -o $@ $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS) Sysy*.* compiler

