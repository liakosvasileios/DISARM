CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Source files
SRC = src/transformer.c src/mutate.c src/decoder.c src/encoder.c src/mba.c src/jit.c src/dispatch.c
TEST_SRC = src/decoder.c src/encoder.c test/test_decode_1.c 

# Output binaries
OUT = build/transform 
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
	gcc -Wall -Wextra -Iinclude src/decoder.c src/encoder.c src/mutate.c src/jit.c src/dispatch.c test/test_mutate.c -o build/test_mutate.exe
	./build/test_mutate.exe

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
	gcc -static -Wl,--strip-all test/generate_pe.c -o test_binaries/test.exe

test-dispatch:
	gcc -Wall -Wextra -Iinclude -o test_binaries/dispatch_test test/dispatch_test.c src/dispatch.c
	./test_binaries/dispatch_test

jit: 
	gcc -Wall -Wextra -Iinclude -o build/jit src/jit.c src/dispatch.c src/decoder.c src/encoder.c
	./build/jit

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

main:
	gcc -Wall -Wextra -Iinclude -o build/disarm src/main.c
	./build/disarm

transform:
	./build/transform.exe test_binaries/input.bin test_binaries/output.bin 0

pe-test:
	gcc -Wall -Wextra -Iinclude -o build/pe_test src/pe_parser.c test/test_pe_parser.c
	./build/pe_test test_binaries/test.exe

pe-patch:
	gcc -Wall -Wextra -Iinclude -o build/patch src/pe_parser.c src/patch_pe_text.c src/decoder.c src/encoder.c src/mutate.c src/jit.c src/dispatch.c
	./build/patch.exe test_binaries/test.exe test_binaries/test_mut.exe 0 0

patch:
	gcc -Wall -Wextra -Iinclude -o build/patcher src/pe_parser.c src/patch_pe_text.c test/test_pe_parser.c
	./build/patcher.exe test_binaries/test.exe

.PHONY: all test clean
