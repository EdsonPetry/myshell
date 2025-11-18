#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024 // 1kb

//finds if a file exists 
char *findFunction(char *function) {
  char buffer[BUFFER_SIZE];
  if (access("/usr/local/bin", F_OK) == 0) {
    snprintf(buffer, BUFFER_SIZE, "/usr/local/bin/%s", function);
    return buffer;
  } else if (access("/usr/bin", F_OK) == 0) {
    snprintf(buffer, BUFFER_SIZE, "/usr/bin/%s", function);
    return buffer;
  } else if (access("/bin", F_OK) == 0) {
    printf(buffer, BUFFER_SIZE, "/bin/%s", function);
    return buffer;
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
  if (buffer == NULL) {
    perror("malloc failed");
    return EXIT_FAILURE;
  }
  if (getcwd(buffer, BUFFER_SIZE) != 0) {
    free(buffer);
    return EXIT_FAILURE;
  }
  write(fd, buffer, BUFFER_SIZE);
  free(buffer);
  return EXIT_SUCCESS;
}

int which(char *function) {
  //If the argument to which is a builtin function, fail
  if (strcmp(function, "cd") == 0 || strcmp(function, "pwd") == 0 || strcmp(function, "which") == 0 || strcmp(function, "exit") == 0 || strcmp(function, "die") == 0) {
    return EXIT_FAILURE;
  } else {
    char *path = findFunction(function);
    if (path == NULL){
      printf("command not found");
      return EXIT_FAILURE;
    }
    printf(path);
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
2: exit
3: die
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

  int num_commands = parsed_command->num_commands;
  int read_fd = STDIN_FILENO;
  if (parsed_command->input_file != NULL) {
    read_fd = open(parsed_command->input_file, O_RDONLY);
  }
  int pid;
  //int output_fd = open(parsed_command->output_file);

  int final_state = EXIT_SUCCESS;
  
  Command *commands_list = parsed_command->commands;
  int pfd[2];
  for (int i = 0; i < num_commands; i++) {
    if (i + 1 < num_commands) {
      if (pipe(pfd) != 0) {
        perror("Pipe failed");
        return EXIT_FAILURE;
      }
    }
    pid = fork();
    if (pid == 0) {
      //child

      if (read_fd != STDIN_FILENO) {
        dup2(read_fd, STDIN_FILENO);
        close(read_fd);
      }

      if (i < num_commands - 1) {
        //write to pipe if there's more commands
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[0]);
        close(pfd[1]);
      } else {
        //last command will send output to the output
        if (parsed_command != NULL) {
          int write_fd = open(parsed_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
          dup2(write_fd, STDOUT_FILENO);
          close(write_fd);
        }
      }
      switch (whichFunction(commands_list[i].args[0])) {
        case 0:
          char *path = findFunction(commands_list[i].args[0]);
          execv(path, ++commands_list[i].args);
          return EXIT_FAILURE;
        case 1:
          if (commands_list[i].num_args != 2) {
            perror("cd got too many arguments");
            return EXIT_FAILURE;
          }
          if (cd(commands_list[i].args[1])) {
            return EXIT_FAILURE;
          }
          break;
        case 2:
          if (commands_list[i].num_args != 1) {
            perror("pwd does not accept arguments");
            return EXIT_FAILURE;
          }
          if (pwd(pfd[1])) {
            perror("pwd failed");
            return EXIT_FAILURE;
          }
          break;
        case 3:
          if (commands_list[i].num_args != 2) {
            perror("which got a wrong number of variables");
            return EXIT_FAILURE;
          }
          printf("%s\n", whichFunction(commands_list[i].args[1]));
        case 4:
          return EXIT_SUCCESS;
        case 5:
          for (int j = 1; j < commands_list[i].num_args; j++) {
            printf("%s ", commands_list[i].args[j]);
          }
          printf("\n");
          return 1;
        default:
          //something bad happened
          return EXIT_FAILURE;
      }

    } else {
      if (read_fd != STDIN_FILENO) {
        close(read_fd);
      }
      if (i < num_commands - 1) {
        close(pfd[1]);
        read_fd = pfd[0];
      }
    }
      
  }



    
}

