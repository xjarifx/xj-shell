# xjsh Architecture

This document describes the internal architecture of xjsh and how the major modules collaborate at runtime.

## 1. Design Goals

- Keep implementation simple and explicit.
- Favor direct POSIX syscalls over shell delegation.
- Keep module boundaries clear (`main`, `parser`, `builtins`, `history`).
- Return deterministic status codes for easy testing and scripting.

## 2. High-Level Architecture

xjsh follows a single-process interactive loop:

1. Read a line from stdin.
2. Parse line into a `Command` (`argc` + `argv`).
3. Dispatch command to built-in handler.
4. Print output/errors.
5. Repeat until `exit` or EOF.

There is no fork/exec pipeline in current architecture.

## 3. Runtime Control Flow

Primary control loop is implemented in `src/main.c`.

### 3.1 Startup

- Initialize line buffer (`line = NULL`, `getline`-managed).
- Initialize `History` via `history_init`.
- Set `last_status = 0`.

### 3.2 Per-Iteration Steps

1. Print prompt `xjsh> `.
2. Read line via `getline`.
3. On EOF/error (`nread < 0`): print newline and break loop.
4. Strip trailing newline if present.
5. Ignore empty line.
6. Record line in history (`history_add`).
7. Parse into command (`parse_command`).
8. On parse failure: set status `2`, continue loop.
9. Execute command (`execute_command`).
10. Free parsed command (`free_command`).
11. If `exit` requested, break with requested status.

### 3.3 Shutdown

- Free history (`history_free`).
- Free line buffer.
- Return `last_status` from `main`.

## 4. Module Responsibilities

### 4.1 `main` module (`src/main.c`)

- Owns shell lifecycle.
- Coordinates prompt, read, parse, execute.
- Owns process-level exit status policy.

### 4.2 `parser` module (`src/parser.c`)

- Converts raw input line into `Command` structure.
- Allocates token memory dynamically.
- Handles quoting and escaping rules.
- Provides cleanup API to avoid memory leaks.

Interface in `include/parser.h`:

- `int parse_command(const char *line, Command *out_cmd)`
- `void free_command(Command *cmd)`

### 4.3 `builtins` module (`src/builtins.c`)

- Provides command dispatch table via if-chain in `execute_command`.
- Implements each built-in command handler.
- Encapsulates POSIX-level filesystem operations.
- Produces normalized return codes across handlers.

Interface in `include/builtins.h`:

- `int execute_command(int argc, char *argv[], History *history, int *should_exit)`
- `void print_help(void)`

### 4.4 `history` module (`src/history.c`)

- Stores command lines in process memory.
- Uses growable array strategy (`capacity *= 2`).
- Prints numbered history view.
- Cleans up all allocated entries.

Interface in `include/history.h`:

- `void history_init(History *history)`
- `int history_add(History *history, const char *line)`
- `void history_print(const History *history)`
- `void history_free(History *history)`

## 5. Data Model

### 5.1 `Command`

Defined in `include/parser.h`:

- `char **argv`: NULL-terminated vector of argument strings.
- `int argc`: Number of arguments.

Invariants:

- On success, `argv[argc] == NULL`.
- Ownership of `argv[i]` and `argv` is transferred to caller.
- Caller must invoke `free_command`.

### 5.2 `History`

Defined in `include/history.h`:

- `char **entries`: dynamic array of command strings.
- `size_t count`: number of active entries.
- `size_t capacity`: current allocation size.

Growth behavior:

- Initial capacity is 16 on first insertion.
- Reallocation doubles capacity.

## 6. Parser Semantics

Parser is a single-pass state machine over input characters.

State flags:

- `in_single`: inside `'...'`
- `in_double`: inside `"..."`

Rules:

- Whitespace splits tokens only when not in quote mode.
- `'` toggles single-quote mode when not in double-quote mode.
- `"` toggles double-quote mode when not in single-quote mode.
- Backslash escapes next character outside single quotes.
- Unterminated quote causes parse error and command reset.

Memory behavior:

- Temporary token buffer grows geometrically.
- Each completed token is copied into heap-owned arg string.
- On any allocation failure, parser frees partial state and returns error.

## 7. Command Dispatch and Execution

`execute_command` validates and routes `argv[0]`.

Dispatch strategy:

- Linear `strcmp` chain for command name matching.
- Unknown command prints error and returns `127`.

Exit handling:

- `exit` is handled as regular built-in.
- Handler sets `*should_exit` to requested status.
- Main loop observes flag and terminates cleanly.

## 8. Built-in Command Implementation Map

- `whereami`: `getcwd`
- `showfiles`: `opendir`, `readdir`, `closedir`
- `enter`: `chdir`
- `move`/`rename`: `rename`, fallback `link` + `unlink` on `EXDEV`
- `delete`: `lstat`, interactive prompt, then `unlink` or `rmdir`
- `create`: `open(O_CREAT|O_WRONLY|O_TRUNC, 0644)`, `close`
- `makedir`: `mkdir(0755)`
- `removedir`: `rmdir`
- `read`: `open`, `read`, `write`, `close`
- `write`: `open(O_CREAT|O_WRONLY|O_APPEND, 0644)`, `write`, `close`
- `history`: `history_print`
- `help`: static usage text
- `exit`: parse optional status (`atoi`)

## 9. Error Handling Strategy

- Syscall failures are reported with `perror`.
- Usage mistakes return status `2` and print usage text.
- Unknown command returns `127`.
- Most handlers return immediately on first fatal error.
- Cleanup is explicit in each failure path.

## 10. Memory and Resource Management

- `getline` buffer is reused across loop iterations and freed once.
- Command allocations are per-command and freed after dispatch.
- History entries persist until shell exit.
- File descriptors and directory handles are explicitly closed.

Potential caveat:

- In `move` cross-device fallback, if `link` succeeds and `unlink` fails, source and destination may both exist.

## 11. Security and Safety Characteristics

- No `system()` or shell command injection surface from built-ins.
- `delete` requires explicit user confirmation.
- Operations run with current process permissions (no privilege escalation).
- No sandboxing is implemented.

## 12. Build Architecture

Build is managed by `Makefile`:

- Compiler: `cc`
- Standard: `-std=c11`
- Warnings: `-Wall -Wextra -Wpedantic`
- Optimization: `-O2`
- Includes: `-Iinclude`
- Sources linked into single binary `xjsh`

## 13. Extension Points

To add a new built-in command:

1. Add static handler function in `src/builtins.c`.
2. Validate `argc` and usage contract.
3. Return standardized status codes.
4. Add `strcmp` branch in `execute_command`.
5. Add help text in `print_help`.
6. Document command in README.

To evolve parser behavior:

1. Update state machine in `src/parser.c`.
2. Preserve `Command` ownership contract.
3. Add tests for edge cases (quotes, escapes, empty args).

## 14. Current Limitations and Future Architecture Directions

Limitations:

- No external process execution.
- No redirection/pipes/job control.
- No persistent history file.
- No recursive delete.

Future architectural enhancements:

- Introduce command registry table instead of linear if-chain.
- Add executor module for external commands (`fork`/`execvp`/`waitpid`).
- Add parser AST for pipelines and redirections.
- Add configuration and startup script handling.
- Add automated test harness around parser and built-ins.
