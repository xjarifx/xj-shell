# Custom OS Shell in C

A custom command-line shell written in C for an Operating Systems project. The shell exposes its own command language, performs file and directory operations through OS interfaces, and launches programs through an explicit `execve`-based execution path instead of acting like a thin Bash alias layer.

## Project Structure

- `src/main.c` - REPL loop, signal setup, background child reaping
- `src/parser.c` / `src/parser.h` - tokenizer and command parser
- `src/builtins.c` / `src/builtins.h` - custom shell command implementations
- `src/executor.c` / `src/executor.h` - pipeline, redirection, fork/exec orchestration
- `src/os_utils.c` / `src/os_utils.h` - low-level write helpers and executable resolution
- `src/history.c` / `src/history.h` - command history storage
- `src/shell.h` - shared shell data structures
- `Makefile` - compile and run helpers
- `scripts/test_shell.sh` - smoke test

## Features

### Core Features
- REPL loop with prompt (`nova> `)
- User input reading with `getline`
- Tokenization and parsing of commands
- Process creation and synchronization with `fork`, `execve`, and `waitpid`
- Error handling for malformed commands and failed system calls

### Custom Command Language
This shell has its own command vocabulary.

| Command | Purpose | OS/API used |
| --- | --- | --- |
| `drift <dir>` | change current directory | `chdir()` |
| `ground` | print current directory | `getcwd()` |
| `forge <file>` | create or clear a file | `open()` |
| `inscribe <file> <text>` | write text into a file | `open()`, `write()` |
| `etch <file> <text>` | append text to a file | `open()`, `write()` |
| `peek <file>` | display file contents | `open()`, `read()`, `write()` |
| `vanish <file>` | delete a file | `unlink()` |
| `nest <dir>` | create a directory | `mkdir()` |
| `sweep [dir]` | list directory contents | `opendir()`, `readdir()` |
| `recall` | show command history | in-memory history array |
| `summon <program> [args]` | execute an external program | `fork()`, path resolution, `execve()` |
| `help` | print help | built-in |
| `quit` | exit the shell | built-in |

### Advanced Features
- Pipes: `|`
- Input redirection: `<`
- Output redirection: `>`
- Append redirection: `>>`
- Background execution: `&`
- Command history
- Background child reaping with `SIGCHLD`

## Build

```bash
cd /home/x/Documents/os-custom-shell
make
```

## Run

```bash
./customsh
```

## Example Session

```text
nova> ground
/home/x/Documents/os-custom-shell

nova> forge notes.txt
nova> inscribe notes.txt this shell talks to the os directly
nova> peek notes.txt
this shell talks to the os directly

nova> nest demo
nova> sweep
Makefile
README.md
customsh
notes.txt
scripts
src
demo

nova> summon ls -l
... external program output ...

nova> quit
```

## Architecture

### 1. REPL Layer
The shell follows a read-eval-print loop:
1. display prompt
2. read one line of input
3. store the line in history
4. tokenize and parse it
5. execute the resulting command structure

This loop keeps the shell interactive.

### 2. Parser Layer
The tokenizer scans the raw input and separates it into tokens such as:
- command names
- arguments
- quoted strings
- pipe operators `|`
- redirection operators `<`, `>`, `>>`
- background operator `&`

This stage turns free-form text into structured pieces the parser can understand.

The parser converts tokens into a `ParsedLine` structure containing:
- one or more `Command` objects
- each command's `argv` argument array
- optional input redirection file
- optional output redirection file
- append flag for `>>`
- background execution flag

If the parser detects missing filenames, invalid pipeline structure, duplicate redirects, trailing pipes, or misplaced `&`, it reports an error.

### 3. Built-in Commands and Direct OS Interaction
Most of the custom shell language is implemented as built-ins that call operating system services directly.

Examples:
- `drift` uses `chdir()` to update the shell's working directory in the parent process
- `forge` uses `open()` with `O_CREAT | O_TRUNC` to create or reset a file
- `peek` uses `open()`, `read()`, and `write()` to print file content without calling `cat`
- `sweep` uses `opendir()` and `readdir()` to inspect directories without calling `ls`
- `vanish` uses `unlink()` instead of calling `rm`

This is the key difference from a shell that simply renames Bash commands.

### 4. External Program Execution
External programs are only launched through the explicit custom command:

```text
summon <program> [args]
```

For example:

```text
summon gcc main.c -o app
summon ./app
```

The shell handles this by:
1. calling `fork()`
2. in the child, applying any redirection with `dup2()`
3. resolving the executable path itself
4. calling `execve()` on the requested program
5. in the parent, waiting with `waitpid()` unless it is a background job

This keeps the shell language distinct while still allowing access to normal system programs.

### 5. Execution Layer
The executor module is responsible for:
- parent-side built-in execution when shell state must persist, such as `drift`
- child execution for background jobs and pipelines
- descriptor rewiring for redirection
- process creation and synchronization for pipelines

This makes the shell easier to extend without pushing parsing and OS execution back into one file.

### 6. Pipelines and Redirection
If multiple commands are connected with pipes:
- the shell creates pipes with `pipe()`
- forks one child per pipeline stage
- connects stdin/stdout using `dup2()`
- closes unused pipe descriptors
- waits for all children after execution

Redirection works by opening the requested file and replacing standard input or standard output using `dup2()`.

### 7. Background Process Management
If a command ends with `&`, the shell:
- starts the child process
- returns control to the prompt immediately
- later reaps finished children using `waitpid(..., WNOHANG)`
- uses `SIGCHLD` to know cleanup is needed

### 8. History and Shell State
Command history is managed by its own module and stored in a shared `ShellState` structure. That gives the shell a clearer place to grow stateful features later.

### 9. Error Handling
The shell reports errors for:
- unterminated quotes
- invalid pipelines
- missing redirect targets
- invalid built-in usage
- failed file operations
- failed process creation or execution
- unknown commands that are not built-ins and not introduced with `summon`

## System Calls and Interfaces Used
- `getline()`
- `fork()`
- `execve()`
- `waitpid()`
- `pipe()`
- `dup2()`
- `open()`
- `read()`
- `write()`
- `close()`
- `chdir()`
- `getcwd()`
- `unlink()`
- `mkdir()`
- `opendir()` / `readdir()`
- `access()`
- `sigaction()`

## Testing
Run the smoke test:

```bash
bash scripts/test_shell.sh
```

## Why This Design Matches the Requirement Better
This shell does not depend on Bash syntax or simple alias mapping for its main behavior.

- Its command language is custom.
- File and directory operations are handled directly in C.
- The codebase is split into parser, builtins, executor, and OS utility layers instead of one monolithic source file.
- External programs are resolved explicitly and launched through `execve()`.
- Core OS concepts like processes, descriptors, redirection, and synchronization are exposed clearly in the implementation.

That makes it a stronger Operating Systems project because it demonstrates direct control over system-level behavior.
