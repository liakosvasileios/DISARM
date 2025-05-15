CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Source files
SRC = main.c src/decoder.c src/encoder.c
TEST_SRC = src/decoder.c src/encoder.c test/test_decode_1.c 

# Output binaries
OUT = build/morpher.exe
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
	gcc -Wall -Wextra -Iinclude src/instruction_codec.c src/mutate.c test/test_mutate.c -o build/test_mutate.exe
	./build/test_mutate.exe

transform:
	gcc -Wall -Wextra -Iinclude src/transformer.c src/mutate.c src/instruction_codec.c src/mba.c -o build/transform

clean:
	@echo Cleaning build...
	@rm -f build/*.exe

dump: 
	@echo "[*] Disassembly of original input:"
	@objdump -D -b binary -m i386:x86-64 test_binaries/input.bin | head -n 100
	@echo ""
	@echo "[*] Disassembly of transformed output:"
	@objdump -D -b binary -m i386:x86-64 test_binaries/output.bin | head -n 100

input: 
	gcc -Wall -Wextra -Iinclude -o test_binaries/generate_input test/generate_input.c
	./test_binaries/generate_input

deepfry:
	@echo "[*] Mutating a clean output.bin..."
	./build/transform.exe test_binaries/input.bin test_binaries/output.bin 1;
	@echo "[*] Deepfrying output.bin $(n) times..."
	@if [ -z "$(n)" ]; then \
		echo "Usage: make deepfry n=5"; \
		exit 1; \
	fi; \
	for i in $(shell seq 1 $(n)); do \
		echo "  → Pass $$i"; \
		cp test_binaries/output.bin test_binaries/tmp_input.bin; \
		./build/transform.exe test_binaries/tmp_input.bin test_binaries/output.bin 1; \
	done; \
	rm -f test_binaries/tmp_input.bin	

.PHONY: all test clean
