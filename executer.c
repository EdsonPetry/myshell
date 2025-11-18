#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024 // 1kb

//finds if a file exists 
char *findFunction(char *function) {
  if (access("/usr/local/bin", F_OK)) {
    return ("/usr/local/bin/%s", function);
  } else if (access("/usr/bin", F_OK)) {
    return ("/usr/bin/%s", function);
  } else if (access("/bin", F_OK)) {
    return ("/bin/%s", function);
  } else {
    return NULL;
  }
}

int cd(char *destination) { 
  if (chdir(destination) != 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int pwd(int fd) {
  char *buffer = malloc(BUFFER_SIZE * sizeof(char));
  if (getcwd(buffer, BUFFER_SIZE) != 0) {
    return EXIT_FAILURE;
  }
  write(fd, buffer, BUFFER_SIZE);
  return EXIT_SUCCESS;
}

int which(char *function) {
  //If the argument to which is a builtin function, fail
  if (strcmp(function, "cd") == 0 || strcmp(function, "pwd") == 0 || strcmp(function, "which") == 0 || strcmp(function, "exit") == 0 || strcmp(function, "die") == 0) {
    return EXIT_FAILURE;
  } else {
    printf(findFunction(function));
  }
  return EXIT_SUCCESS;
}

/*
If the command is not a builtin function, the function will return 0, otherwise, it will return the following:
1 - cd
2 - pwd
3 - which
4 - exit
5 - die
*/
int whichFunction(char *command) {
  if (strcmp(command, "cd") == 0) {
    return 1;
  } else if (strcmp(command, "pwd") == 0) {
    return 2;
  } else if (strcmp(command, "which") == 0) {
    return 3;
  } else if (strcmp(command, "exit") == 0) {
    return 4;
  } else if (strcmp(command, "die") == 0) {
    return 5;
  } else {
    return 0;
  }
}

/* 
Possible return status are: 
0: success 
1: failure
*/
int execute(ParsedCmd *parsed_command, int prevState, int is_interactive, int fd) {
  if (prevState == EXIT_SUCCESS && parsed_command->is_or == 1) {
    return prevState;
  }
  if (prevState == EXIT_FAILURE && parsed_command->is_and == 1) {
    return prevState;
  }
  if (parsed_command == NULL) {
    return prevState;
  }

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

