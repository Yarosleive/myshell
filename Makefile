CFLAGS = -std=c23 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -Iinc

all: main

main: src/*.c inc/*.h
	gcc $(CFLAGS) -o $@ src/*.c

debug: CFLAGS += -g -fsanitize=address,undefined
debug: clean main

clean:
	rm -f main
