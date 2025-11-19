#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024 // 1kb

char *BUILTIN[] = {"cd", "pwd", "which", "exit", "die"};

//finds if a file exists 
char *findFunction(char *function) {
  if (strchr(function, '/') != NULL) {
    if (access(function, X_OK) == 0) {
      char *result = malloc(strlen(function) + 1);
      if (result == NULL) {
        return NULL;
      }
      strcpy(result, function);
      return result;
    }
  }
  char buffer[BUFFER_SIZE];
  char *directories[] = {"/usr/local/bin", "/usr/bin", "/bin"};
  for (int i = 0; i < 3; i++) {
    snprintf(buffer, BUFFER_SIZE, "%s/%s", directories[i], function);
    // check that the file exists and is executable
    if (access(buffer, X_OK) == 0) {
      char *result = malloc(strlen(buffer) + 1);
      if (result == NULL) {
        return NULL;
      }
      strcpy(result, buffer);
      return result;
    }
  }

  return NULL;
}

int cd(char *destination) { 
  if (chdir(destination) != 0) {
    perror("chdir failed");
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
  if (getcwd(buffer, BUFFER_SIZE) == NULL) {
    free(buffer);
    return EXIT_FAILURE;
  }
  write(fd, buffer, strlen(buffer));
  write(fd, "\n", 1);
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
      return EXIT_FAILURE;
    }
    printf("%s\n", path);
    free(path);
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
int execute(ParsedCmd *parsed_command, int prevState, int is_interactive, int *should_exit) {
    
  if (parsed_command == NULL) {
    return prevState;
  }

  if (prevState == EXIT_SUCCESS && parsed_command->is_or == 1) {
    return prevState;
  }
  if (prevState == EXIT_FAILURE && parsed_command->is_and == 1) {
    return prevState;
  }

  int num_commands = parsed_command->num_commands;
  int read_fd = STDIN_FILENO;
  if (parsed_command->input_file != NULL) {
    read_fd = open(parsed_command->input_file, O_RDONLY);
    if (read_fd < 0) {
      perror("input file");
      return EXIT_FAILURE;
    }
  }

  Command *commands_list = parsed_command->commands;
  int output_fd = STDOUT_FILENO;
  if (parsed_command->output_file != NULL) {
    output_fd = open(parsed_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (output_fd < 0) {
      perror("can't open output file");
      if (read_fd != STDIN_FILENO) close(read_fd);
      return EXIT_FAILURE;
    }
  }

  //one function
  if (num_commands == 1) {
    switch (whichFunction(commands_list[0].args[0]))
    {
    case 1:
      //cd accepts one argument
      if (commands_list[0].num_args != 2) {
        printf("too many args in cd\n");
        return EXIT_FAILURE;
      }
      if (cd(commands_list[0].args[1]) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
      return EXIT_SUCCESS;
      break;
    
    case 2:
    //pwd accepts no arguments
      if (commands_list[0].num_args != 1) {
        printf("too many args in pwd\n");
        return EXIT_FAILURE;
      }
      if (pwd(output_fd) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
      return EXIT_SUCCESS;
      break;
    
    case 3:
    //which accepts one argument
      if (commands_list[0].num_args != 2) {
        printf("which only takes one argument\n");
        return EXIT_FAILURE;
      }
      if (which(commands_list[0].args[1]) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
      return EXIT_SUCCESS;
      break;

    case 4:
      //exit doesn't care, exit is god, it succeeds :)
      if (commands_list[0].num_args != 1) {
        printf("exit takes no arguments\n");
        return EXIT_FAILURE;
      }
      *should_exit = 1;
      return EXIT_SUCCESS;
      break;
    
    case 5:
      //die will print all argument and fail, it is not god :(
      *should_exit = 1;
      for (int j = 1; j < commands_list[0].num_args; j++) {
        if (j > 1) printf(" ");
        printf("%s", commands_list[0].args[j]);
      }
      if (commands_list[0].num_args > 1) printf("\n");
      return EXIT_FAILURE;
      break;
    
    case 0:
      //holy uncharted territory
      pid_t pid = fork();
      if (pid == 0) {
        //child
        if (read_fd != STDIN_FILENO) {
          dup2(read_fd, STDIN_FILENO);
          close(read_fd);
        }
        if (output_fd != STDOUT_FILENO) {
          dup2(output_fd, STDOUT_FILENO);
          close(output_fd);
        }
        char *path = findFunction(commands_list[0].args[0]);
        if (path == NULL) {
          printf("command not found\n");
          exit(EXIT_FAILURE);
        }
        execv(path, commands_list[0].args);
        return EXIT_FAILURE;
      } else  {
        //parent
        int status;
        wait(&status);
        if (WIFEXITED(status)) {
          return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
          return EXIT_FAILURE;
        } else {
          return EXIT_FAILURE;
        }
      }
    default:
      break;
    }
  }

  //more than one command
  pid_t pid;
  int pfd[2];
  int last_status = 0;
  for (int i = 0; i < num_commands; i++) {
    if (i < num_commands - 1) {
      if (pipe(pfd) != 0) {
        perror("pipe");
        if (read_fd != STDIN_FILENO) close(read_fd);
        return EXIT_FAILURE;
      }
    }

    pid = fork();
    if (pid < 0) {
      perror("fork");
      return EXIT_FAILURE;
    }

    if (pid == 0) {
      //child
      if (read_fd != STDIN_FILENO) {
        dup2(read_fd, STDIN_FILENO);
        close(read_fd);
      }

      if (i < num_commands - 1) {
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[0]);
        close(pfd[1]);
      } else {
        if (parsed_command->output_file != NULL) {
          int write_fd = open(parsed_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
          if (write_fd < 0) {
            perror("output file");
            exit(EXIT_FAILURE);
          }
          dup2(write_fd, STDOUT_FILENO);
          close(write_fd);
        }
      }


      switch (whichFunction(commands_list[i].args[0])) {
        case 0:
            char *path = findFunction(commands_list[i].args[0]);
            if (path == NULL) {
              printf("command not found\n");
              exit(EXIT_FAILURE);
            }
            execv(path, commands_list[i].args);
            perror("execv");
            free(path);
            exit(EXIT_FAILURE);
        case 1:
          if (commands_list[i].num_args != 2) {
            printf("cd got too many arguments\n");
            exit(EXIT_FAILURE);
          }
          if (cd(commands_list[i].args[1]) != EXIT_SUCCESS) {
            exit(EXIT_FAILURE);
          }
          exit(EXIT_SUCCESS);
        case 2:
          if (commands_list[i].num_args != 1) { 
            exit(EXIT_FAILURE); 
          }
          if (pwd(STDOUT_FILENO) != EXIT_SUCCESS)  {
            exit(EXIT_FAILURE);
          }
          exit(EXIT_SUCCESS);
        case 3:
          if (commands_list[i].num_args != 2) {
            exit(EXIT_FAILURE);
          }
          {
            char *path = findFunction(commands_list[i].args[1]);
            if (path == NULL) {
              printf("command not found\n");
              exit(EXIT_FAILURE);
            }
            printf("%s\n", path);
            free(path);
            exit(EXIT_SUCCESS);
          }
        case 4:
          exit(EXIT_SUCCESS);
        case 5:
          for (int j = 1; j < commands_list[i].num_args; j++) {
            if (j > 1) printf(" ");
            printf("%s", commands_list[i].args[j]);
          }
          if (commands_list[i].num_args > 1) printf("\n");
          exit(EXIT_FAILURE);
        default:
          exit(EXIT_FAILURE);
        }
    } else {
      //parent
      if (read_fd != STDIN_FILENO) {
        close(read_fd);
      }
      if (i < num_commands - 1) {
        close(pfd[1]);
        read_fd = pfd[0];
      }
      int status;
      waitpid(pid, &status, 0);
      if (WIFEXITED(status)) {
        last_status = WEXITSTATUS(status);
      } else {
        last_status = EXIT_FAILURE;
      }
    }
  }
  if (output_fd != STDOUT_FILENO) {
    close(output_fd);
  }
  return last_status;
}

