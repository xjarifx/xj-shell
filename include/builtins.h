#ifndef BUILTINS_H
#define BUILTINS_H

#include "history.h"

int execute_command(int argc, char *argv[], History *history, int *should_exit);
void print_help(void);

#endif
