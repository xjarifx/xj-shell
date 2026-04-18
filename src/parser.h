#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

void parser_init_parsed_line(struct ParsedLine *parsed);
int parser_tokenize(const char *line, char **tokens, int max_tokens);
void parser_free_tokens(char **tokens, int count);
int parser_parse_tokens(char **tokens, int token_count, struct ParsedLine *parsed);

#endif
