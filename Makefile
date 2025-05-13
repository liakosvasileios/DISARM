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

transform:
	gcc -Wall -Wextra -Iinclude src/transformer.c src/mutate.c src/decode.c src/mba.c -o build/transform

clean:
	@echo Cleaning build...
	@rm -f build/*.exe

dump: 
	@echo "[*] Disassembly of original input:"
	@objdump -D -b binary -m i386:x86-64 test_binaries/input.bin | head -n 40
	@echo ""
	@echo "[*] Disassembly of transformed output:"
	@objdump -D -b binary -m i386:x86-64 test_binaries/output.bin | head -n 40

input: 
	gcc -Wall -Wextra -Iinclude -o test_binaries/generate_input test/generate_input.c
	./test_binaries/generate_input

.PHONY: all test clean
