#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

typedef struct {
    char **argv;
    int argc;
} Command;

int parse_command(const char *line, Command *out_cmd);
void free_command(Command *cmd);

#endif
