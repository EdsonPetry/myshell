#ifndef PARSER_H
#define PARSER_H

typedef struct ParsedCmd ParsedCmd;

ParsedCmd *parse(const char *line);
void free_parsed_cmd(ParsedCmd *cmd);
#endif
