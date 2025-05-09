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

test-decode: $(TEST_OUT)
	$(TEST_OUT)

$(TEST_OUT): $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) -o $(TEST_OUT)

test-mutate:
	gcc -Wall -Wextra -Iinclude src/decode.c src/mutate.c test/test_mutate.c -o build/test_mutate.exe
	./build/test_mutate.exe

clean:
	@echo Cleaning build...
	@rm -f build/*.exe

.PHONY: all test clean
