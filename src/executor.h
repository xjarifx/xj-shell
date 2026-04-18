#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "shell.h"

int executor_execute(struct ShellState *state, struct ParsedLine *parsed);

#endif
