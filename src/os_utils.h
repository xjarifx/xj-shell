#ifndef OS_UTILS_H
#define OS_UTILS_H

#include <sys/types.h>

ssize_t os_write_all(int fd, const void *buffer, size_t count);
char *os_resolve_executable(const char *program);

#endif
