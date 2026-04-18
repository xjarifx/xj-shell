#define _POSIX_C_SOURCE 200809L

#include "builtins.h"
#include "history.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char *line = NULL;
    size_t line_cap = 0;
    int last_status = 0;
    History history;

    history_init(&history);

    for (;;) {
        ssize_t nread;
        Command cmd;
        int exit_status = -1;

        fputs("xjsh> ", stdout);
        fflush(stdout);

        nread = getline(&line, &line_cap, stdin);
        if (nread < 0) {
            putchar('\n');
            break;
        }

        if (nread > 0 && line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        if (line[0] == '\0') {
            continue;
        }

        if (history_add(&history, line) != 0) {
            fprintf(stderr, "warning: failed to store history\n");
        }

        if (parse_command(line, &cmd) != 0) {
            last_status = 2;
            continue;
        }

        last_status = execute_command(cmd.argc, cmd.argv, &history, &exit_status);
        free_command(&cmd);

        if (exit_status >= 0) {
            last_status = exit_status;
            break;
        }
    }

    history_free(&history);
    free(line);
    return last_status;
}
