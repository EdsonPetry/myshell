#include "dynamic_array.h"
#include "parser.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// tokenize returns a dynamic array of tokens from a string input
// tokens are substrings separated by whitespace
Array tokenize(const char *input) {
  Array tokens;

  initArray(&tokens, 10);

  int i = 0;

  while (input[i] != '\0') {
    if (input[i] == ' ' || input[i] == '\t') {
      i++;
      continue;
    }

    if (input[i] == '<' || input[i] == '>' || input[i] == '|') {
      char *token = (char *)malloc(sizeof(char) * 2);
      token[0] = input[i];
      token[1] = '\0';
      insertArray(&tokens, token);
      i++;
      continue;
    }

    // found a token
    int token_start = i;
    while (input[i] != ' ' && input[i] != '\t' && input[i] != '\0' &&
           input[i] != '<' && input[i] != '>' && input[i] != '|') {
      i++;
    }

    char *token = (char *)malloc(sizeof(char) * (i - token_start + 1));
    strncpy(token, input + token_start, i - token_start);
    token[i - token_start] = '\0';
    insertArray(&tokens, token);
  }

  return tokens;
}

// copy of strdup
char *my_strdup(const char *s) {
  if (s == NULL) {
    return NULL;
  }

  size_t len = strlen(s);
  char *new_s = (char *)malloc(len + 1);

  if (new_s == NULL) {
    return NULL;
  }

  strcpy(new_s, s);
  return new_s;
}

ParsedCmd *parse(const char *line) {
  if (line == NULL) {
    return NULL;
  }

  int length = strlen(line);
  int comment_pos = length;
  for (int i = 0; i < length; i++) {
    if (line[i] == '#') {
      comment_pos = i;
      break;
    }
  }

  char *cleaned_line = (char *)malloc(sizeof(char) * (comment_pos + 1));
  if (comment_pos > 0) {
    strncpy(cleaned_line, line, comment_pos);
  }

  cleaned_line[comment_pos] = '\0';

  Array tokens = tokenize(cleaned_line);

  ParsedCmd *parsed_cmd = (ParsedCmd *)malloc(sizeof(ParsedCmd));
  parsed_cmd->input_file = NULL;
  parsed_cmd->output_file = NULL;
  parsed_cmd->is_and = 0;
  parsed_cmd->is_or = 0;
  parsed_cmd->num_commands = 0;

  // check first token for conditional
  int token_i = 0;
  if (tokens.used > 0) {
    if (strcmp(tokens.array[0], "and") == 0) {
      parsed_cmd->is_and = 1;
      token_i = 1;
    } else if (strcmp(tokens.array[0], "or") == 0) {
      parsed_cmd->is_or = 1;
      token_i = 1;
    }
  }

  // check if empty cmd
  if (token_i >= tokens.used) {
    free(parsed_cmd);
    freeArray(&tokens);
    free(cleaned_line);
    return NULL;
  }

  parsed_cmd->commands = malloc(sizeof(Command) * 10);
  int cmd_i = 0;

  parsed_cmd->commands[cmd_i].args = malloc(sizeof(char *) * 10);
  parsed_cmd->commands[cmd_i].num_args = 0;
  parsed_cmd->num_commands = 1;

  for (int i = token_i; i < tokens.used; i++) {
    char *token = tokens.array[i];

    if (strcmp(token, "<") == 0) {
      i++;
      if (i >= tokens.used) {
        // error: missing filename after <
        free_parsed_cmd(parsed_cmd);
        freeArray(&tokens);
        free(cleaned_line);
        return NULL;
      }

      char *filename = tokens.array[i];
      if (strcmp(filename, "<") == 0 || strcmp(filename, ">") == 0 ||
          strcmp(filename, "|") == 0) {
        free_parsed_cmd(parsed_cmd);
        freeArray(&tokens);
        free(cleaned_line);
        return NULL;
      }

      parsed_cmd->input_file = my_strdup(filename);

    } else if (strcmp(token, ">") == 0) {
      i++;
      if (i >= tokens.used) {
        // error: missing filename after >
        free_parsed_cmd(parsed_cmd);
        freeArray(&tokens);
        free(cleaned_line);
        return NULL;
      }

      char *filename = tokens.array[i];
      if (strcmp(filename, "<") == 0 || strcmp(filename, ">") == 0 ||
          strcmp(filename, "|") == 0) {
        free_parsed_cmd(parsed_cmd);
        freeArray(&tokens);
        free(cleaned_line);
        return NULL;
      }

      parsed_cmd->output_file = my_strdup(filename);

    } else if (strcmp(token, "|") == 0) {
      // pipeline starts a new command

      if (parsed_cmd->commands[cmd_i].num_args == 0) {
        free_parsed_cmd(parsed_cmd);
        freeArray(&tokens);
        free(cleaned_line);
        return NULL;
      }

      // null-terminate current command's args
      parsed_cmd->commands[cmd_i].args[parsed_cmd->commands[cmd_i].num_args] =
          NULL;

      cmd_i++;
      parsed_cmd->commands[cmd_i].args = malloc(sizeof(char *) * 10);
      parsed_cmd->commands[cmd_i].num_args = 0;
      parsed_cmd->num_commands = cmd_i + 1;

    } else {
      // regular argument
      Command *curr = &parsed_cmd->commands[cmd_i];
      curr->args[curr->num_args] = my_strdup(token);
      curr->num_args++;
    }
  }

  if (parsed_cmd->commands[cmd_i].num_args == 0) {
    free_parsed_cmd(parsed_cmd);
    freeArray(&tokens);
    free(cleaned_line);
    return NULL;
  }

  // null terminate last command's args
  parsed_cmd->commands[cmd_i].args[parsed_cmd->commands[cmd_i].num_args] = NULL;
  parsed_cmd->num_commands = cmd_i + 1;

  freeArray(&tokens);
  free(cleaned_line);

  return parsed_cmd;
}

void free_parsed_cmd(ParsedCmd *cmd) {
  if (cmd == NULL) {
    return;
  }

  for (int i = 0; i < cmd->num_commands; i++) {
    for (int j = 0; j < cmd->commands[i].num_args; j++) {
      free(cmd->commands[i].args[j]);
    }
    free(cmd->commands[i].args);
  }

  if (cmd->commands != NULL) {
    free(cmd->commands);
  }

  if (cmd->input_file != NULL) {
    free(cmd->input_file);
  }
  if (cmd->output_file != NULL) {
    free(cmd->output_file);
  }

  free(cmd);
}
