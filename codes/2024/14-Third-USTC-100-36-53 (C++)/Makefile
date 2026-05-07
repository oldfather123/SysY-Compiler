CC = g++
CFLAGS = -Wall -ggdb # -O2

SRC = $(wildcard src/**/*.cpp src/**/*.c)
INC = $(wildcard include/**)

PREFIX =
TARGET = $(PREFIX)compiler


all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS) $(addprefix -I, $(INC))

clean:
	rm -f $(TARGET)


.PHONY: all clean
