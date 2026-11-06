CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -Iinclude -std=c11
TARGET = scout
SRC = src/main.c src/explorer.c
LIBS = -lncursesw

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET) scout-fm

.PHONY: all clean
