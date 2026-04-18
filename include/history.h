#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

typedef struct {
    char **entries;
    size_t count;
    size_t capacity;
} History;

void history_init(History *history);
int history_add(History *history, const char *line);
void history_print(const History *history);
void history_free(History *history);

#endif
