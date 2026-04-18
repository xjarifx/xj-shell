#define _POSIX_C_SOURCE 200809L

#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void history_init(History *history) {
    history->entries = NULL;
    history->count = 0;
    history->capacity = 0;
}

int history_add(History *history, const char *line) {
    char *copy;

    if (!history || !line || line[0] == '\0') {
        return 0;
    }

    if (history->count == history->capacity) {
        size_t new_capacity = history->capacity == 0 ? 16 : history->capacity * 2;
        char **new_entries = realloc(history->entries, new_capacity * sizeof(char *));
        if (!new_entries) {
            perror("realloc");
            return -1;
        }
        history->entries = new_entries;
        history->capacity = new_capacity;
    }

    copy = strdup(line);
    if (!copy) {
        perror("strdup");
        return -1;
    }

    history->entries[history->count++] = copy;
    return 0;
}

void history_print(const History *history) {
    size_t i;

    if (!history) {
        return;
    }

    for (i = 0; i < history->count; ++i) {
        printf("%zu  %s\n", i + 1, history->entries[i]);
    }
}

void history_free(History *history) {
    size_t i;

    if (!history) {
        return;
    }

    for (i = 0; i < history->count; ++i) {
        free(history->entries[i]);
    }
    free(history->entries);

    history->entries = NULL;
    history->count = 0;
    history->capacity = 0;
}
