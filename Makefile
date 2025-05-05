CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Source files
SRC = main.c src/decode.c
TEST_SRC = src/decode.c test/test_decode_1.c 

# Output binaries
OUT = build/S3gM0rph.exe
TEST_OUT = build/test_decode_1.exe

# Default target
all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

test: $(TEST_OUT)
	$(TEST_OUT)

$(TEST_OUT): $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) -o $(TEST_OUT)

clean:
	@echo Cleaning build...
	-del /Q build\*.exe 2>nul || true

.PHONY: all test clean
