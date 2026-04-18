#define _POSIX_C_SOURCE 200809L

#include "builtins.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int cmd_whereami(int argc, char *argv[]) {
    char cwd[PATH_MAX];
    (void)argv;

    if (argc != 1) {
        fprintf(stderr, "usage: whereami\n");
        return 2;
    }

    if (!getcwd(cwd, sizeof(cwd))) {
        perror("getcwd");
        return 1;
    }

    puts(cwd);
    return 0;
}

static int cmd_showfiles(int argc, char *argv[]) {
    DIR *dir;
    struct dirent *entry;
    (void)argv;

    if (argc != 1) {
        fprintf(stderr, "usage: showfiles\n");
        return 2;
    }

    dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        puts(entry->d_name);
    }

    if (closedir(dir) != 0) {
        perror("closedir");
        return 1;
    }

    return 0;
}

static int cmd_enter(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: enter <path>\n");
        return 2;
    }

    if (chdir(argv[1]) != 0) {
        perror("chdir");
        return 1;
    }

    return 0;
}

static int move_fallback_link_unlink(const char *src, const char *dst) {
    if (link(src, dst) != 0) {
        perror("link");
        return 1;
    }

    if (unlink(src) != 0) {
        perror("unlink");
        return 1;
    }

    return 0;
}

static int cmd_move(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: move <source> <destination>\n");
        return 2;
    }

    if (rename(argv[1], argv[2]) == 0) {
        return 0;
    }

    if (errno == EXDEV) {
        return move_fallback_link_unlink(argv[1], argv[2]);
    }

    perror("rename");
    return 1;
}

static int cmd_delete(int argc, char *argv[]) {
    struct stat st;
    char answer[8];

    if (argc != 2) {
        fprintf(stderr, "usage: delete <path>\n");
        return 2;
    }

    if (lstat(argv[1], &st) != 0) {
        perror("lstat");
        return 1;
    }

    printf("delete '%s'? [y/N]: ", argv[1]);
    fflush(stdout);
    if (!fgets(answer, sizeof(answer), stdin)) {
        fprintf(stderr, "delete canceled\n");
        return 1;
    }

    if (!(answer[0] == 'y' || answer[0] == 'Y')) {
        fprintf(stderr, "delete canceled\n");
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        if (rmdir(argv[1]) != 0) {
            perror("rmdir");
            return 1;
        }
    } else {
        if (unlink(argv[1]) != 0) {
            perror("unlink");
            return 1;
        }
    }

    return 0;
}

static int cmd_create(int argc, char *argv[]) {
    int fd;

    if (argc != 2) {
        fprintf(stderr, "usage: create <filename>\n");
        return 2;
    }

    fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (close(fd) != 0) {
        perror("close");
        return 1;
    }

    return 0;
}

static int cmd_makedir(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: makedir <dirname>\n");
        return 2;
    }

    if (mkdir(argv[1], 0755) != 0) {
        perror("mkdir");
        return 1;
    }

    return 0;
}

static int cmd_removedir(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: removedir <dirname>\n");
        return 2;
    }

    if (rmdir(argv[1]) != 0) {
        perror("rmdir");
        return 1;
    }

    return 0;
}

static int cmd_read(int argc, char *argv[]) {
    int fd;
    ssize_t n;
    char buf[4096];

    if (argc != 2) {
        fprintf(stderr, "usage: read <filename>\n");
        return 2;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t m = write(STDOUT_FILENO, buf + written, (size_t)(n - written));
            if (m < 0) {
                perror("write");
                close(fd);
                return 1;
            }
            written += m;
        }
    }

    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    if (close(fd) != 0) {
        perror("close");
        return 1;
    }

    return 0;
}

static int cmd_write(int argc, char *argv[]) {
    int fd;
    int i;

    if (argc < 3) {
        fprintf(stderr, "usage: write <filename> <text...>\n");
        return 2;
    }

    fd = open(argv[1], O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    for (i = 2; i < argc; ++i) {
        const char *part = argv[i];
        size_t len = strlen(part);
        size_t written = 0;

        while (written < len) {
            ssize_t n = write(fd, part + written, len - written);
            if (n < 0) {
                perror("write");
                close(fd);
                return 1;
            }
            written += (size_t)n;
        }

        if (i + 1 < argc) {
            if (write(fd, " ", 1) != 1) {
                perror("write");
                close(fd);
                return 1;
            }
        }
    }

    if (write(fd, "\n", 1) != 1) {
        perror("write");
        close(fd);
        return 1;
    }

    if (close(fd) != 0) {
        perror("close");
        return 1;
    }

    return 0;
}

void print_help(void) {
    puts("Custom shell commands:");
    puts("  whereami                    Show current working directory");
    puts("  showfiles                   List files in current directory");
    puts("  enter <path>                Change working directory");
    puts("  move <source> <dest>        Move or rename a path");
    puts("  rename <old> <new>          Rename a path");
    puts("  delete <path>               Delete file or empty directory (with confirmation)");
    puts("  create <filename>           Create empty file");
    puts("  makedir <dirname>           Create directory");
    puts("  removedir <dirname>         Remove empty directory");
    puts("  read <filename>             Print file contents");
    puts("  write <filename> <text...>  Append text to file");
    puts("  history                     Show command history");
    puts("  help                        Show this help message");
    puts("  exit                        Exit shell");
}

int execute_command(int argc, char *argv[], History *history, int *should_exit) {
    (void)history;

    if (argc == 0) {
        return 0;
    }

    if (strcmp(argv[0], "whereami") == 0) {
        return cmd_whereami(argc, argv);
    }
    if (strcmp(argv[0], "showfiles") == 0) {
        return cmd_showfiles(argc, argv);
    }
    if (strcmp(argv[0], "enter") == 0) {
        return cmd_enter(argc, argv);
    }
    if (strcmp(argv[0], "move") == 0) {
        return cmd_move(argc, argv);
    }
    if (strcmp(argv[0], "rename") == 0) {
        return cmd_move(argc, argv);
    }
    if (strcmp(argv[0], "delete") == 0) {
        return cmd_delete(argc, argv);
    }
    if (strcmp(argv[0], "create") == 0) {
        return cmd_create(argc, argv);
    }
    if (strcmp(argv[0], "makedir") == 0) {
        return cmd_makedir(argc, argv);
    }
    if (strcmp(argv[0], "removedir") == 0) {
        return cmd_removedir(argc, argv);
    }
    if (strcmp(argv[0], "read") == 0) {
        return cmd_read(argc, argv);
    }
    if (strcmp(argv[0], "write") == 0) {
        return cmd_write(argc, argv);
    }
    if (strcmp(argv[0], "help") == 0) {
        print_help();
        return 0;
    }
    if (strcmp(argv[0], "history") == 0) {
        history_print(history);
        return 0;
    }
    if (strcmp(argv[0], "exit") == 0) {
        if (argc > 2) {
            fprintf(stderr, "usage: exit [status]\n");
            return 2;
        }
        if (argc == 2) {
            *should_exit = atoi(argv[1]);
        } else {
            *should_exit = 0;
        }
        return 0;
    }

    fprintf(stderr, "unknown command: %s\n", argv[0]);
    return 127;
}
