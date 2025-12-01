CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -pthread

TARGET = trabalho_final
SOURCES = main.c structures.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = structures.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) 5 10

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all run clean
