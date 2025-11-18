#include "parser.h"
#include "executor.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024 // 1kb

int main(int argc, char *argv[]) {
  int input_fd;

  if (argc == 1) {
    // no input, so enter interactive mode
    input_fd = STDIN_FILENO;
  } else if (argc == 2) {
    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
      // error: couldn't open file
      perror("mysh");
      return EXIT_FAILURE;
    }
  }

  int is_interactive = isatty(input_fd);
  char buffer[BUFFER_SIZE];
  int buffer_len = 0;

  if (is_interactive) {
    printf("Welcome to mysh!\n");
  }

  while (true) {
    if (is_interactive) {
      printf("mysh> ");
      fflush(stdout);
    }

    // read input
    ssize_t bytes_read = read(input_fd, buffer + buffer_len, BUFFER_SIZE - buffer_len);

    if (bytes_read == 0) {
      // EOF
      break;
    }

    if (bytes_read < 0) {
      // error in read
      perror("read");
      break;
    }

    buffer_len += bytes_read;

    while (1) {
      char *newline_pos = memchr(buffer, '\n', buffer_len);

      if (newline_pos == NULL) {
        break;
      }

      int line_len = newline_pos - buffer;
      char cmd_line[line_len + 1];
      memcpy(cmd_line, buffer, line_len);
      cmd_line[line_len] = '\0';

      // Place holder for parse/execute
      printf("Got command: [%s]\n", cmd_line);

      // TODO: implement
      ParsedCmd *cmd = parse(cmd_line);

      int finalState = execute(cmd, 0, is_interactive, input_fd);
      // TODO: implement
      // execute cmd here

      // TODO: implement
      /* free_parsed_cmd(cmd); */

      free_parsed_cmd(cmd);
      // check for exit/die

      if (finalState == 2) {
        return EXIT_SUCCESS;
      } else if (finalState == 3) {
        return EXIT_FAILURE;
      }

      // shift buffer
      int remaining = buffer_len - (line_len + 1);
      memmove(buffer, newline_pos + 1, remaining);
      buffer_len = remaining;
    }

    if (buffer_len >= BUFFER_SIZE) {
      // error: line too long
      break;
    }
  }

  if (is_interactive) {
    printf("Goodbye!\n");
  }

  if (argc == 2) {
    close(input_fd);
  }

  return EXIT_SUCCESS;
}
