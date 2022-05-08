CC=gcc

CFLAGS=-Wall -Wconversion -g -Og

emulator: src/emulator.c
	$(CC) $(CFLAGS) -o $@ src/$@.c

.PHONY: clean
clean:
	rm emulator || true
