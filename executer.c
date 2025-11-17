#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024 // 1kb

typedef struct {
  char **args;
  int num_args;
} Command;

typedef struct ParsedCmd {
  Command *commands;
  int num_commands;

  char *input_file;
  char *output_file;

  int is_and;
  int is_or;
} ParsedCmd;


/* 
Possible return status are: 
0: success 
1: failure
*/
int execute(ParsedCmd *parsed_command, int is_interactive, int fd) {
  int final_state = 0;
  Command *commands_list = parsed_command->commands;
  for (int i = 0; i < parsed_command->num_commands; i++) {
      if (commands_list[i].num_args != 0) {
          if (strcmp(commands_list[i].args[0], "cd")) {
            if (commands_list[i].num_args != 2) {
              perror("cd got too many arguments");
              return 1;
            }
            if (chdir(commands_list[i].args[1]) != 0) {
              perror("cd failed");
              return 1;
            }
          } else if (strcmp(commands_list[i].args[0], "pwd")) {
            if (commands_list[i].num_args != 1) {
              perror("pwd does not accept arguments");
              return 1;
            }
            char *buffer = malloc(BUFFER_SIZE * sizeof(char));
            if (buffer == NULL) {
              perror("malloc failed");
              return 1;
            }
            if (getcwd(buffer, BUFFER_SIZE) != 0) {
              perror("get cwd failed");
              return 1;
            }
            //later, will need to configure how to put the output in a pipe
            printf("%s", buffer);
          } else if (strcmp(commands_list[i].args[0], "which")) {
            //build a command to find where 
          } else if (strcmp(commands_list[i].args[0], "exit")) {
            return 0;
          } else if (strcmp(commands_list[i].args[0], "die")) {
            for (int j = 1; j < commands_list[i].num_args; j++) {
              printf("%s ", commands_list[i].args[j]);
            }
            printf("\n");
            return 1;
          } else {

          }
      }
  }



    
}

//builtin commands:

int cd(char **path) {
    if (chdir(path) != 0) {
        return 1;
    }
    return 0;
}

