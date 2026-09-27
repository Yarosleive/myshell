main: src/*.c inc/*.h
	gcc -Wall -Wextra -g -Iinc src/*.c -o main
