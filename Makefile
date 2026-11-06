CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
TARGET = scout-fm
SRC = src/main.c src/explorer.c
LIBS = -lncursesw

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean