CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -pthread

TARGET = trabalho_final
SOURCES = main.c structures.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = structures.h

# Test sources
TEST_SOURCES = test_centralized_control_mechanism.c
TEST_BIN = test_ccm
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) 5 10

# Build test binary
test: $(TEST_BIN)

$(TEST_BIN): $(TEST_SOURCES) structures.c $(HEADERS)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SOURCES) structures.c $(LDFLAGS)

# Build and run tests
test-run: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all run clean
