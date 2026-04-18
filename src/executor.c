#define _POSIX_C_SOURCE 200809L

#include "executor.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins.h"
#include "os_utils.h"

extern char **environ;

static void setup_redirection(struct Command *cmd) {
    if (cmd->input_file != NULL) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd == -1) {
            perror(cmd->input_file);
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2 input");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);
    }

    if (cmd->output_file != NULL) {
        int flags = O_CREAT | O_WRONLY | (cmd->append_output ? O_APPEND : O_TRUNC);
        int fd = open(cmd->output_file, flags, 0644);
        if (fd == -1) {
            perror(cmd->output_file);
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2 output");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);
    }
}

static int run_external_command(struct Command *cmd) {
    if (strcmp(cmd->argv[0], "summon") != 0) {
        fprintf(stderr,
                "Unknown command: %s\nUse 'summon <program> [args]' for external programs.\n",
                cmd->argv[0]);
        return 1;
    }

    if (cmd->argv[1] == NULL) {
        fprintf(stderr, "summon: missing program name\n");
        return 1;
    }

    char *resolved_path = os_resolve_executable(cmd->argv[1]);
    if (resolved_path == NULL) {
        perror(cmd->argv[1]);
        return 1;
    }

    execve(resolved_path, &cmd->argv[1], environ);

    int saved_errno = errno;
    free(resolved_path);
    errno = saved_errno;
    perror("summon");
    return 1;
}

static int run_command_in_child(struct ShellState *state, struct Command *cmd) {
    setup_redirection(cmd);

    if (cmd->argv[0] == NULL) {
        return 0;
    }

    if (builtin_is_name(cmd->argv[0])) {
        bool should_exit = false;
        return builtin_dispatch(state, cmd, &should_exit);
    }

    return run_external_command(cmd);
}

static int execute_pipeline(struct ShellState *state, struct ParsedLine *parsed) {
    int pipefds[2 * (MAX_SEGMENTS - 1)] = {0};
    pid_t pids[MAX_SEGMENTS] = {0};

    for (int i = 0; i < parsed->command_count - 1; i++) {
        if (pipe(pipefds + i * 2) == -1) {
            perror("pipe");
            return 1;
        }
    }

    for (int i = 0; i < parsed->command_count; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return 1;
        }

        if (pids[i] == 0) {
            signal(SIGINT, SIG_DFL);

            if (i > 0 && dup2(pipefds[(i - 1) * 2], STDIN_FILENO) == -1) {
                perror("dup2 pipe input");
                exit(EXIT_FAILURE);
            }
            if (i < parsed->command_count - 1 &&
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO) == -1) {
                perror("dup2 pipe output");
                exit(EXIT_FAILURE);
            }

            for (int j = 0; j < 2 * (parsed->command_count - 1); j++) {
                close(pipefds[j]);
            }

            exit(run_command_in_child(state, &parsed->commands[i]));
        }
    }

    for (int i = 0; i < 2 * (parsed->command_count - 1); i++) {
        close(pipefds[i]);
    }

    if (parsed->background) {
        printf("[bg] started pipeline with lead pid %d\n", pids[0]);
        return 0;
    }

    int final_status = 0;
    for (int i = 0; i < parsed->command_count; i++) {
        int status = 0;
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
            final_status = 1;
            continue;
        }
        if (i == parsed->command_count - 1) {
            if (WIFEXITED(status)) {
                final_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                final_status = 128 + WTERMSIG(status);
            }
        }
    }

    return final_status;
}

static int execute_single_command(struct ShellState *state, struct ParsedLine *parsed) {
    struct Command *cmd = &parsed->commands[0];

    if (cmd->argv[0] == NULL) {
        return 0;
    }

    if (!parsed->background && builtin_is_name(cmd->argv[0]) &&
        cmd->input_file == NULL && cmd->output_file == NULL) {
        bool should_exit = false;
        int status = builtin_dispatch(state, cmd, &should_exit);
        if (should_exit) {
            state->should_exit = true;
        }
        return status;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        exit(run_command_in_child(state, cmd));
    }

    if (parsed->background) {
        printf("[bg] started pid %d\n", pid);
        return 0;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 1;
}

int executor_execute(struct ShellState *state, struct ParsedLine *parsed) {
    if (parsed->command_count == 0) {
        return 0;
    }

    if (parsed->command_count == 1) {
        return execute_single_command(state, parsed);
    }

    return execute_pipeline(state, parsed);
}
