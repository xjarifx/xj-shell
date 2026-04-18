#define _POSIX_C_SOURCE 200809L

#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int push_arg(Command *cmd, const char *token, size_t len) {
    char *arg;
    char **new_argv;

    arg = malloc(len + 1);
    if (!arg) {
        perror("malloc");
        return -1;
    }
    memcpy(arg, token, len);
    arg[len] = '\0';

    new_argv = realloc(cmd->argv, (size_t)(cmd->argc + 2) * sizeof(char *));
    if (!new_argv) {
        perror("realloc");
        free(arg);
        return -1;
    }

    cmd->argv = new_argv;
    cmd->argv[cmd->argc++] = arg;
    cmd->argv[cmd->argc] = NULL;
    return 0;
}

int parse_command(const char *line, Command *out_cmd) {
    const char *p = line;
    char *token_buf = NULL;
    size_t cap = 0;
    size_t len = 0;
    int in_single = 0;
    int in_double = 0;

    out_cmd->argv = NULL;
    out_cmd->argc = 0;

    while (*p != '\0') {
        char c = *p;

        if (!in_single && !in_double && isspace((unsigned char)c)) {
            if (len > 0) {
                if (push_arg(out_cmd, token_buf, len) != 0) {
                    free(token_buf);
                    free_command(out_cmd);
                    return -1;
                }
                len = 0;
            }
            ++p;
            continue;
        }

        if (!in_double && c == '\'') {
            in_single = !in_single;
            ++p;
            continue;
        }

        if (!in_single && c == '"') {
            in_double = !in_double;
            ++p;
            continue;
        }

        if (!in_single && c == '\\' && p[1] != '\0') {
            ++p;
            c = *p;
        }

        if (len + 1 >= cap) {
            size_t new_cap = cap == 0 ? 64 : cap * 2;
            char *new_buf = realloc(token_buf, new_cap);
            if (!new_buf) {
                perror("realloc");
                free(token_buf);
                free_command(out_cmd);
                return -1;
            }
            token_buf = new_buf;
            cap = new_cap;
        }

        token_buf[len++] = c;
        ++p;
    }

    if (in_single || in_double) {
        fprintf(stderr, "parse error: unmatched quote\n");
        free(token_buf);
        free_command(out_cmd);
        return -1;
    }

    if (len > 0) {
        if (push_arg(out_cmd, token_buf, len) != 0) {
            free(token_buf);
            free_command(out_cmd);
            return -1;
        }
    }

    free(token_buf);
    return 0;
}

void free_command(Command *cmd) {
    int i;

    if (!cmd || !cmd->argv) {
        return;
    }

    for (i = 0; i < cmd->argc; ++i) {
        free(cmd->argv[i]);
    }
    free(cmd->argv);
    cmd->argv = NULL;
    cmd->argc = 0;
}
