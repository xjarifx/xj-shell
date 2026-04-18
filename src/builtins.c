#define _POSIX_C_SOURCE 200809L

#include "builtins.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "history.h"
#include "os_utils.h"

static int builtin_drift(char **argv) {
    const char *path = argv[1] ? argv[1] : getenv("HOME");
    if (path == NULL) {
        fprintf(stderr, "drift: HOME not set\n");
        return 1;
    }
    if (chdir(path) != 0) {
        perror("drift");
        return 1;
    }
    return 0;
}

static int builtin_ground(void) {
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        perror("ground");
        return 1;
    }
    printf("%s\n", cwd);
    free(cwd);
    return 0;
}

static int builtin_forge(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "forge: missing filename\n");
        return 1;
    }

    int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("forge");
        return 1;
    }

    close(fd);
    return 0;
}

static int write_joined_arguments(int fd, char **argv, int start_index, const char *label) {
    for (int i = start_index; argv[i] != NULL; i++) {
        if (os_write_all(fd, argv[i], strlen(argv[i])) < 0) {
            perror(label);
            return 1;
        }

        if (argv[i + 1] != NULL && os_write_all(fd, " ", 1) < 0) {
            perror(label);
            return 1;
        }
    }

    if (os_write_all(fd, "\n", 1) < 0) {
        perror(label);
        return 1;
    }

    return 0;
}

static int builtin_inscribe(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "inscribe: usage inscribe <file> <text>\n");
        return 1;
    }

    int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("inscribe");
        return 1;
    }

    int status = write_joined_arguments(fd, argv, 2, "inscribe");
    close(fd);
    return status;
}

static int builtin_etch(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "etch: usage etch <file> <text>\n");
        return 1;
    }

    int fd = open(argv[1], O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd == -1) {
        perror("etch");
        return 1;
    }

    int status = write_joined_arguments(fd, argv, 2, "etch");
    close(fd);
    return status;
}

static int builtin_peek(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "peek: missing filename\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("peek");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes = 0;
    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        if (os_write_all(STDOUT_FILENO, buffer, (size_t)bytes) < 0) {
            perror("peek");
            close(fd);
            return 1;
        }
    }

    if (bytes < 0) {
        perror("peek");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}

static int builtin_vanish(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "vanish: missing path\n");
        return 1;
    }

    if (unlink(argv[1]) != 0) {
        perror("vanish");
        return 1;
    }
    return 0;
}

static int builtin_nest(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "nest: missing directory name\n");
        return 1;
    }

    if (mkdir(argv[1], 0755) != 0) {
        perror("nest");
        return 1;
    }
    return 0;
}

static int builtin_sweep(char **argv) {
    const char *path = argv[1] ? argv[1] : ".";
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("sweep");
        return 1;
    }

    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 0;
}

static int builtin_recall(struct ShellState *state) {
    history_print(&state->history);
    return 0;
}

static int builtin_help(void) {
    printf("Custom shell commands:\n");
    printf("  drift <dir>            Change current directory\n");
    printf("  ground                 Print current directory\n");
    printf("  forge <file>           Create or clear a file\n");
    printf("  inscribe <file> <txt>  Write text into a file\n");
    printf("  etch <file> <txt>      Append text into a file\n");
    printf("  peek <file>            Print file contents\n");
    printf("  vanish <file>          Delete a file\n");
    printf("  nest <dir>             Create a directory\n");
    printf("  sweep [dir]            List directory contents\n");
    printf("  recall                 Show command history\n");
    printf("  summon <prog> [args]   Launch an OS program with execve\n");
    printf("  help                   Show this help\n");
    printf("  quit                   Exit shell\n\n");
    printf("Supports pipes (|), redirection (<, >, >>), and background jobs (&).\n");
    return 0;
}

bool builtin_is_name(const char *name) {
    static const char *const builtins[] = {
        "drift", "ground", "forge", "inscribe", "etch", "peek",
        "vanish", "nest", "sweep", "recall", "help", "quit"
    };

    if (name == NULL) {
        return false;
    }

    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strcmp(name, builtins[i]) == 0) {
            return true;
        }
    }

    return false;
}

int builtin_dispatch(struct ShellState *state, struct Command *cmd, bool *should_exit) {
    if (cmd->argv[0] == NULL) {
        return 0;
    }

    if (should_exit != NULL) {
        *should_exit = false;
    }

    if (strcmp(cmd->argv[0], "drift") == 0) {
        return builtin_drift(cmd->argv);
    }
    if (strcmp(cmd->argv[0], "ground") == 0) {
        return builtin_ground();
    }
    if (strcmp(cmd->argv[0], "forge") == 0) {
        return builtin_forge(cmd->argv);
    }
    if (strcmp(cmd->argv[0], "inscribe") == 0) {
        return builtin_inscribe(cmd->argv);
    }
    if (strcmp(cmd->argv[0], "etch") == 0) {
        return builtin_etch(cmd->argv);
    }
    if (strcmp(cmd->argv[0], "peek") == 0) {
        return builtin_peek(cmd->argv);
    }
    if (strcmp(cmd->argv[0], "vanish") == 0) {
        return builtin_vanish(cmd->argv);
    }
    if (strcmp(cmd->argv[0], "nest") == 0) {
        return builtin_nest(cmd->argv);
    }
    if (strcmp(cmd->argv[0], "sweep") == 0) {
        return builtin_sweep(cmd->argv);
    }
    if (strcmp(cmd->argv[0], "recall") == 0) {
        return builtin_recall(state);
    }
    if (strcmp(cmd->argv[0], "help") == 0) {
        return builtin_help();
    }
    if (strcmp(cmd->argv[0], "quit") == 0) {
        if (should_exit != NULL) {
            *should_exit = true;
        }
        return 0;
    }

    fprintf(stderr, "Unknown built-in command: %s\n", cmd->argv[0]);
    return 1;
}
