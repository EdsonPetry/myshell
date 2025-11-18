CC = gcc
CFLAGS = -g -Wall -Wvla -std=c99 -fsanitize=address,undefined
DEBUG_OBJS = my_shell_debug.o
REGULAR_OBJS = my_shell.o parser.o dynamic_array.o
TEST_OBJS = test_parser.o parser.o dynamic_array.o
TEST_EXECUTOR_OBJS = test_executor.o parser.o dynamic_array.o

regular: $(REGULAR_OBJS)
	$(CC) $(CFLAGS) $^ -o mysh

debug: $(DEBUG_OBJS)
	$(CC) $(CFLAGS) $^ -o mysh_debug

test_parser: $(TEST_OBJS)
	$(CC) $(CFLAGS) $^ -o test_parser -lcunit
	./test_parser

test_executor: $(TEST_EXECUTOR_OBJS)
	$(CC) $(CFLAGS) $^ -o test_executor
	./test_executor

test_all: test_parser test_executor

%_debug.o: %.c
	$(CC) $(CFLAGS) -DDEBUG=1 -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o mysh mysh_debug test_parser test_executor
