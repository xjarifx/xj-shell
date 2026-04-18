#ifndef HISTORY_H
#define HISTORY_H

#include "shell.h"

void history_add(struct History *history, const char *line);
void history_print(const struct History *history);
void history_free(struct History *history);

#endif
