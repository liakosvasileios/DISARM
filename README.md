## DISARM: Detection Inhibition via Self-Adaptive Runtime Mutation
- Use `make all` to compile the code. (No testing)
- Use `make test-decode` to test the encoder-decoder with a round trip test. (Change the last argument of the `test` function to 1 *verbose* if you want to read the contents of the instructions encoded-decoded)
- Use `make test-mutate` to test the mutations **manually**. 
- Use `./build/transform.exe test_binaries/input.bin test_binaries/output.bin 1` to mutate the input.bin file. (0 for deterministic, 1 for random)
- Use `make dump` to generate the dump files.
