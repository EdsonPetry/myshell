CC = gcc
CFLAGS = -g -Wall -Wvla -std=c99 -fsanitize=address,undefined
DEBUG_OBJS = my_shell_debug.o
REGULAR_OBJS = my_shell.o parser.o

regular: $(REGULAR_OBJS)
	$(CC) $(CFLAGS) $^ -o mysh

debug: $(DEBUG_OBJS)
	$(CC) $(CFLAGS) $^ -o mysh_debug

%_debug.o: %.c
	$(CC) $(CFLAGS) -DDEBUG=1 -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o mysh mysh_debug
