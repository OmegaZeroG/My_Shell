# My Shell 🐚

A Unix shell implemented from scratch in C, built without using standard string library functions.

## Features

- Interactive prompt with command execution
- Built-in commands
- External command execution via `fork/execve`
- Environment variable management
- PATH resolution
- EOF handling (`Ctrl+D`)

## Built-in Commands

| Command | Description |
|--------|-------------|
| `echo [args]` | Print arguments to stdout. Supports `-n` flag to suppress newline and `$VAR` expansion |
| `pwd` | Print current working directory |
| `cd [directory]` | Change current directory |
| `env` | Print all environment variables |
| `setenv VAR value` | Set an environment variable (`setenv VAR value` or `setenv VAR=value`) |
| `unsetenv VAR` | Remove an environment variable |
| `which command` | *(coming soon)* Locate a command in PATH |
| `exit` / `quit` | Exit the shell |

## External Commands

Any command not recognized as a built-in is searched for across all directories in `PATH` and executed in a child process:

```
[my_shell]$ ls -la
[my_shell]$ cat main.c
[my_shell]$ mkdir testdir
```

## Project Structure

```
.
├── main.c          # Shell loop and built-in dispatcher
├── builtins.c      # Built-in command implementations
├── executor.c      # fork/execve execution, PATH resolution
├── input_parser.c  # Input tokenizer
├── helpers.c       # Custom string library (my_strlen, my_strcmp, etc.)
├── my_shell.h      # Header file with all declarations
└── Makefile
```

## Custom String Library

Built without using `<string.h>`. All string functions are implemented manually in `helpers.c`:

- `my_strlen` — string length
- `my_strcmp` — string comparison
- `my_strncmp` — bounded string comparison
- `my_strdup` — duplicate a string
- `my_strchr` — find character in string
- `my_strcpy` — copy string
- `my_strtok` — tokenize string
- `my_getenv` — look up environment variable

## Getting Started

### Requirements

- Linux or WSL (Windows Subsystem for Linux)
- GCC
- Make

### Build

```bash
make
```

### Run

```bash
./my_shell
```

### Clean

```bash
make fclean   # remove binary
make re       # rebuild from scratch
```

## Example Usage

```
[my_shell]$ echo hello world
hello world

[my_shell]$ echo -n no newline
no newline[my_shell]$

[my_shell]$ setenv MY_VAR hello
[my_shell]$ echo $MY_VAR
hello

[my_shell]$ unsetenv MY_VAR
[my_shell]$ pwd
/mnt/e/Projects/Shell

[my_shell]$ ls -l
total 48
-rw-r--r-- 1 om om  3124 Mar  7 19:00 builtins.c
-rw-r--r-- 1 om om  1893 Mar  7 19:00 executor.c
...

[my_shell]$ exit
```

## Roadmap

- [ ] Arrow key history navigation (↑↓)
- [ ] `which` command implementation
- [ ] Pipe support (`|`)
- [ ] Input/output redirection (`>`, `<`, `>>`)
- [ ] Signal handling (`Ctrl+C`)
- [ ] Quote handling (`"hello world"` as single token)

## Author

**OmegaZeroG** — [github.com/OmegaZeroG](https://github.com/OmegaZeroG)
