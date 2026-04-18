#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "executor.h"
#include "history.h"
#include "parser.h"
#include "shell.h"

static volatile sig_atomic_t reap_requested = 0;

void shell_state_init(struct ShellState *state) {
    memset(state, 0, sizeof(*state));
}

static void sigchld_handler(int signum) {
    (void)signum;
    reap_requested = 1;
}

static void install_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, SIG_IGN);
}

static void reap_background_processes(void) {
    if (!reap_requested) {
        return;
    }

    reap_requested = 0;
    int status = 0;
    pid_t pid = 0;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            printf("[bg] process %d exited with status %d\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[bg] process %d terminated by signal %d\n", pid, WTERMSIG(status));
        }
    }
}

int main(void) {
    struct ShellState state;
    shell_state_init(&state);

    char *line = NULL;
    size_t capacity = 0;

    install_signal_handlers();

    while (!state.should_exit) {
        reap_background_processes();
        printf(PROMPT);
        fflush(stdout);

        ssize_t bytes = getline(&line, &capacity, stdin);
        if (bytes == -1) {
            printf("\n");
            break;
        }

        if (bytes > 0 && line[bytes - 1] == '\n') {
            line[bytes - 1] = '\0';
        }

        if (line[0] == '\0') {
            continue;
        }

        history_add(&state.history, line);

        char *tokens[MAX_TOKENS] = {0};
        int token_count = parser_tokenize(line, tokens, MAX_TOKENS);
        if (token_count < 0) {
            continue;
        }

        struct ParsedLine parsed;
        parser_init_parsed_line(&parsed);

        if (parser_parse_tokens(tokens, token_count, &parsed) == 0) {
            int status = executor_execute(&state, &parsed);
            if (status != 0) {
                fprintf(stderr, "Command finished with status %d\n", status);
            }
        }

        parser_free_tokens(tokens, token_count);
    }

    free(line);
    history_free(&state.history);
    return 0;
}
