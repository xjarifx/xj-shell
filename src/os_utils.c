#define _POSIX_C_SOURCE 200809L

#include "os_utils.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ssize_t os_write_all(int fd, const void *buffer, size_t count) {
    const char *bytes = buffer;
    size_t total = 0;

    while (total < count) {
        ssize_t written = write(fd, bytes + total, count - total);
        if (written < 0) {
            return -1;
        }
        total += (size_t)written;
    }

    return (ssize_t)total;
}

static char *join_path(const char *directory, const char *program) {
    size_t dir_len = strlen(directory);
    size_t program_len = strlen(program);
    size_t needs_slash = dir_len > 0 && directory[dir_len - 1] != '/';
    char *path = malloc(dir_len + needs_slash + program_len + 1);
    if (path == NULL) {
        return NULL;
    }

    memcpy(path, directory, dir_len);
    if (needs_slash) {
        path[dir_len] = '/';
    }
    memcpy(path + dir_len + needs_slash, program, program_len);
    path[dir_len + needs_slash + program_len] = '\0';
    return path;
}

char *os_resolve_executable(const char *program) {
    if (program == NULL || *program == '\0') {
        errno = ENOENT;
        return NULL;
    }

    if (strchr(program, '/') != NULL) {
        if (access(program, X_OK) == 0) {
            return strdup(program);
        }
        return NULL;
    }

    const char *path_env = getenv("PATH");
    if (path_env == NULL || *path_env == '\0') {
        path_env = "/bin:/usr/bin";
    }

    char *path_copy = strdup(path_env);
    if (path_copy == NULL) {
        return NULL;
    }

    for (char *segment = path_copy; segment != NULL;) {
        char *next = strchr(segment, ':');
        if (next != NULL) {
            *next = '\0';
            next++;
        }

        const char *directory = (*segment == '\0') ? "." : segment;
        char *candidate = join_path(directory, program);
        if (candidate == NULL) {
            free(path_copy);
            return NULL;
        }

        if (access(candidate, X_OK) == 0) {
            free(path_copy);
            return candidate;
        }

        free(candidate);
        segment = next;
    }

    free(path_copy);
    errno = ENOENT;
    return NULL;
}
