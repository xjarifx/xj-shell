#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdbool.h>

#include "shell.h"

bool builtin_is_name(const char *name);
int builtin_dispatch(struct ShellState *state, struct Command *cmd, bool *should_exit);

#endif
