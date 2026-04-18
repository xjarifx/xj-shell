#define _POSIX_C_SOURCE 200809L

#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void history_add(struct History *history, const char *line) {
    if (line == NULL || *line == '\0') {
        return;
    }

    char *copy = strdup(line);
    if (copy == NULL) {
        perror("strdup");
        return;
    }

    if (history->count == HISTORY_SIZE) {
        free(history->entries[0]);
        memmove(&history->entries[0],
                &history->entries[1],
                sizeof(char *) * (HISTORY_SIZE - 1));
        history->count--;
    }

    history->entries[history->count++] = copy;
}

void history_print(const struct History *history) {
    for (int i = 0; i < history->count; i++) {
        printf("%d %s\n", i + 1, history->entries[i]);
    }
}

void history_free(struct History *history) {
    for (int i = 0; i < history->count; i++) {
        free(history->entries[i]);
        history->entries[i] = NULL;
    }
    history->count = 0;
}
