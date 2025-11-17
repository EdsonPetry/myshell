#ifndef PARSER_H
#define PARSER_H

typedef struct Command {
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

ParsedCmd *parse(const char *line);
void free_parsed_cmd(ParsedCmd *cmd);

#endif
