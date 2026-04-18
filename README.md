# xjsh

xjsh is a small POSIX-style shell written in C.
It is intentionally focused: all supported commands are built in-process and implemented with libc/POSIX syscalls rather than delegating to another shell.

## What This Project Is

- A learning-oriented shell with a clean `REPL -> parser -> dispatcher` design.
- A practical file-manipulation shell for local filesystem tasks.
- A codebase that demonstrates safe C patterns for dynamic memory, error propagation, and resource cleanup.

## What This Project Is Not

- It is not a full Unix shell replacement.
- It does not currently support pipes, redirection, background jobs, or command substitution.
- It does not execute arbitrary external binaries; only built-in commands are recognized.

## Features

- Interactive prompt loop (`xjsh> `)
- Token parser with:
  - Whitespace tokenization
  - Single-quoted and double-quoted arguments
  - Backslash escaping outside single quotes
- In-memory command history with automatic growth
- Filesystem command suite implemented via POSIX APIs
- Consistent status code semantics across commands
- Defensive command usage checks and syscall error reporting

## Quick Start

### Requirements

- Linux or another Unix-like environment
- C compiler with C11 support
- `make`

### Build

```bash
make
```

Produces the executable:

```bash
./xjsh
```

### Run

```bash
./xjsh
```

Example:

```text
xjsh> whereami
/home/user/project
xjsh> makedir demo
xjsh> enter demo
xjsh> create notes.txt
xjsh> write notes.txt "hello from xjsh"
xjsh> read notes.txt
hello from xjsh
xjsh> history
1  whereami
2  makedir demo
3  enter demo
4  create notes.txt
5  write notes.txt "hello from xjsh"
6  read notes.txt
7  history
xjsh> exit 0
```

## Command Reference

### Navigation and Listing

- `whereami`
  - Prints current working directory.
  - Usage: `whereami`
- `showfiles`
  - Lists entries in the current directory (including `.` and `..` where provided by the filesystem).
  - Usage: `showfiles`
- `enter <path>`
  - Changes the shell process working directory.
  - Usage: `enter /tmp`

### File and Directory Operations

- `move <source> <destination>`
  - Moves or renames a path.
  - Tries `rename(2)` first.
  - If the move crosses filesystems (`EXDEV`), falls back to `link(2)` + `unlink(2)`.
- `rename <old> <new>`
  - Alias of `move`; uses the same implementation and semantics.
- `delete <path>`
  - Deletes file or empty directory after interactive confirmation.
  - Uses `lstat(2)` to detect target type.
  - Cancels unless reply starts with `y` or `Y`.
- `create <filename>`
  - Creates or truncates a file with mode `0644`.
- `makedir <dirname>`
  - Creates a directory with mode `0755`.
- `removedir <dirname>`
  - Removes an empty directory.

### File Content Operations

- `read <filename>`
  - Streams file contents to stdout in chunks.
- `write <filename> <text...>`
  - Appends text to file and adds trailing newline.
  - Multiple text arguments are joined with a single space.

### Shell Control

- `history`
  - Prints command history with 1-based indices.
- `help`
  - Prints built-in command help.
- `exit [status]`
  - Exits shell with optional integer status.
  - If omitted, exits with status `0`.

## Status Codes

- `0`: Success
- `1`: Runtime/system-call failure
- `2`: Parse or usage error
- `127`: Unknown command

Note: `exit [status]` sets the shell process exit code directly.

## Parsing Rules

Input lines are parsed into `argc/argv` using the project parser.

- Spaces delimit arguments unless inside quotes.
- Single quotes `'...'` preserve literal text.
- Double quotes `"..."` group text similarly.
- Backslash escapes the next character outside single quotes.
- Unmatched quote emits `parse error: unmatched quote` and returns status `2`.

## Project Layout

- `src/main.c`: Entry point and REPL loop
- `src/parser.c`: Tokenizer/parser and command memory management
- `src/builtins.c`: Command implementations and command dispatcher
- `src/history.c`: History storage and printing
- `include/parser.h`: Parser interface (`Command`, `parse_command`, `free_command`)
- `include/builtins.h`: Dispatcher interface
- `include/history.h`: History interface
- `Makefile`: Build configuration and targets

## Build Targets

- `make`: Build `xjsh`
- `make run`: Build then run shell
- `make clean`: Remove generated binary

## Architecture Documentation

Detailed internal architecture, module boundaries, and control/data flow are documented in:

- `docs/ARCHITECTURE.md`

## Known Limitations

- No pipeline/redirection syntax (`|`, `>`, `<`, `>>`).
- No environment variable expansion.
- No globbing (`*`, `?`) expansion.
- No external program execution.
- `delete` removes only files and empty directories (not recursive).

## Development Notes

- POSIX feature test macro `_POSIX_C_SOURCE 200809L` is used in source files.
- Error paths are explicit and mostly report via `perror`.
- History is heap-backed and grows geometrically.
- Command parsing allocates per-token memory and frees all via `free_command`.
