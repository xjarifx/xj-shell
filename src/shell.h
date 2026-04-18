#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>

#define MAX_TOKENS 128
#define MAX_SEGMENTS 16
#define HISTORY_SIZE 100
#define BUFFER_SIZE 4096
#define PROMPT "nova> "

struct Command {
    char *argv[MAX_TOKENS];
    char *input_file;
    char *output_file;
    bool append_output;
};

struct ParsedLine {
    struct Command commands[MAX_SEGMENTS];
    int command_count;
    bool background;
};

struct History {
    char *entries[HISTORY_SIZE];
    int count;
};

struct ShellState {
    struct History history;
    bool should_exit;
};

void shell_state_init(struct ShellState *state);

#endif
