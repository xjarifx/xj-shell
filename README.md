# xjsh: Custom POSIX Shell

`xjsh` is a minimal custom command-line interpreter written in C. It directly uses POSIX APIs and does not invoke `bash`, `sh`, `zsh`, `system()`, or external shell tools for built-in commands.

## Features

- Interactive REPL (`read -> parse -> execute -> repeat`)
- Custom parser with support for quoted arguments
- Built-in filesystem commands implemented with POSIX calls
- Relative and absolute path handling through native syscalls
- Error reporting via `perror` and conventional exit codes
- Basic in-memory command history
- `help` command
- Safety confirmation prompt for `delete`

## Command Set

- `whereami`
- `showfiles`
- `enter <path>`
- `move <source> <destination>`
- `rename <old> <new>`
- `delete <path>`
- `create <filename>`
- `makedir <dirname>`
- `removedir <dirname>`
- `read <filename>`
- `write <filename> <text...>`
- `history`
- `help`
- `exit [status]`

## Build

Requirements: a Unix-like system with a C compiler.

```bash
make
```

This produces the executable:

- `./xjsh`

## Run

```bash
./xjsh
```

Example session:

```text
xjsh> whereami
/home/user/project
xjsh> makedir demo
xjsh> enter demo
xjsh> create notes.txt
xjsh> read notes.txt
xjsh> showfiles
.
..
notes.txt
xjsh> exit 0
```

## Implementation Notes

- `whereami`: `getcwd`
- `showfiles`: `opendir`, `readdir`, `closedir`
- `enter`: `chdir`
- `move`/`rename`: `rename`, fallback to `link`+`unlink` on cross-device moves
- `delete`: `lstat`, then `unlink` or `rmdir` with user confirmation
- `create`: `open(..., O_CREAT | O_WRONLY | O_TRUNC, 0644)`
- `makedir`: `mkdir`
- `removedir`: `rmdir`
- `read`: `open`, `read`, `write`
- `write`: `open(..., O_CREAT | O_WRONLY | O_APPEND, 0644)`, `write`

## Exit Codes

- `0`: success
- `1`: runtime/system call failure
- `2`: command usage or parse error
- `127`: unknown command

## Project Structure

- `src/main.c`: REPL loop and shell lifecycle
- `src/parser.c`: tokenizer/parser
- `src/builtins.c`: command handlers and dispatch
- `src/history.c`: in-memory history support
- `include/*.h`: interfaces
