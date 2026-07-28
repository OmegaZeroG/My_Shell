# My Shell 🐚

A POSIX-style Unix shell implemented from scratch in C — no `<string.h>`,
no shell libraries, just `fork`/`execve`/`pipe`/`dup2`/`sigaction` and a
hand-written parser. It supports pipes, I/O redirection, `&&`/`||`
chaining, background jobs, variable expansion, and quoted arguments,
alongside the usual built-ins.

## Features

- Pipelines of any length: `cmd1 | cmd2 | cmd3 | ...`
- I/O redirection: `<`, `>`, `>>`
- Command chaining: `;`, `&&`, `||` (with proper short-circuiting)
- Background jobs: `sleep 10 &`, reaped and reported via `SIGCHLD`
- Variable expansion: `$VAR`, `$?` (last exit status)
- Quoted arguments: `"like this"` or `'like this'`, including glued to
  unquoted text (`foo"bar baz"qux`)
- PATH resolution for external commands, with a fast path for `./foo` /
  `/abs/path`
- Arrow-key (↑↓) command history navigation
- Signal handling: the shell survives `Ctrl+C`/`Ctrl+Z`; a foreground
  child dies normally when interrupted
- Scriptable: `./my_shell < script.sh` or `cmd | ./my_shell` both work,
  falling back to plain line reading when stdin isn't a TTY
- Zero compiler warnings (`-Wall -Wextra -Wshadow -Werror`), zero leaks
  under AddressSanitizer/LeakSanitizer

## Built-in Commands

| Command | Description |
|--------|-------------|
| `echo [args]` | Print arguments to stdout. Supports `-n` to suppress the trailing newline |
| `pwd` | Print current working directory |
| `cd [directory]` | Change current directory (defaults to the shell's start directory) |
| `env` | Print all environment variables |
| `setenv VAR value` / `setenv VAR=value` | Set an environment variable |
| `unsetenv VAR` | Remove an environment variable |
| `which command` | Locate a command in `PATH` |
| `exit` / `quit` | Exit the shell |

`cd`, `setenv`, `unsetenv`, `exit`/`quit` only mutate parent-shell state
when run standalone; used inside a pipe (`cd /tmp | echo hi`) they run in
a throwaway subshell, same as in bash. `echo`, `pwd`, `env`, and `which`
run through the same fork/pipe/redirect machinery as external commands,
so `pwd > out.txt` or `env | grep PATH` work exactly as you'd expect.

## Example Usage

```
[my_shell]$ echo hello world
hello world

[my_shell]$ setenv MY_VAR hello
[my_shell]$ echo $MY_VAR
hello

[my_shell]$ ls | sort | uniq | wc -l
23

[my_shell]$ echo hi > out.txt
[my_shell]$ echo more >> out.txt
[my_shell]$ cat < out.txt
hi
more

[my_shell]$ ls /nonexistent && echo ok || echo fallback
fallback

[my_shell]$ sleep 3 &
[bg] launched pid 4821 (sleep)
[my_shell]$ echo still-responsive
still-responsive
[bg] pid 4821 done: sleep

[my_shell]$ exit
```

## Getting Started

### Requirements

- Linux or WSL (Windows Subsystem for Linux)
- GCC, Make

### Build & Run

```bash
make            # builds with -Wall -Wextra -Wshadow -Werror -g
./my_shell
```

```bash
make fclean     # remove the binary
make re         # rebuild from scratch
```

### Tests

```bash
make && bash tests/test.sh
```

24 regression tests covering built-ins, pipes, redirection, quoting,
`&&`/`||`/`$?`, and parser error handling. Prints PASS/FAIL per case and
exits non-zero if anything fails, so it's CI-ready.

## Project Structure

```
.
├── main.c          # Shell loop: reads input, dispatches pipelines, tracks $?
├── input_parser.c  # Tokenizer + parser (quotes, pipes, redirection, &&/||/;/&) + $VAR expansion
├── executor.c       # fork/pipe/dup2 execution, PATH resolution, redirection
├── builtins.c       # cd, pwd, echo, env, which, setenv, unsetenv
├── signals.c         # SIGINT/SIGTSTP/SIGCHLD setup for parent & child
├── jobs.c            # Background job tracking + SIGCHLD-driven reaping
├── history.c         # Raw-mode input reader, arrow-key history navigation
├── helpers.c          # Custom string library (my_strlen, my_strcmp, etc.)
├── my_shell.h          # Shared structs (Command, Pipeline) and declarations
├── tests/test.sh        # Regression suite
└── Makefile
```

## Custom String Library

Built without `<string.h>`. Every string operation is hand-rolled in
`helpers.c`: `my_strlen`, `my_strcmp`, `my_strncmp`, `my_strdup`,
`my_strchr`, `my_strcpy`, `my_strtok`, `my_getenv`.

## Design Decisions

**Why a `Pipeline`/`Command` split, not a flat argv?** A line like
`a | b > out.txt && c` needs to represent: multiple commands chained by
pipes, each with its own optional redirection, connected to sibling
pipelines by `&&`/`||`/`;`. `Command` owns one program's argv plus its
`<`/`>`/`>>` targets; `Pipeline` owns an ordered list of `Command`s plus
how it connects to the *next* pipeline on the line. Parsing produces an
array of `Pipeline*`, and `main.c`'s dispatch loop walks it left to
right, short-circuiting on `&&`/`||` based on the previous exit status —
the same evaluation order a real shell uses.

**Why do `echo`/`pwd`/`env`/`which` fork instead of running inline?**
So they compose with pipes and redirection like any other command. If
`pwd` ran directly in the parent, `pwd > out.txt` would have to `dup2`
the shell's *own* stdout — permanently redirecting every future prompt.
Forking isolates the redirection to that one command, exactly like a
real external program. `cd`/`setenv`/`unsetenv`/`exit` are the exception:
their entire purpose is to mutate parent-shell state, so they only run
un-forked, and only when they're the sole command with no redirection.

**Why `_exit()` instead of `exit()` in a forked built-in?** This one cost
real debugging time. `exit()` flushes *and closes* every open stdio
stream, including `stdin`. If the shell reads its script from a file
(`./my_shell < script.sh`), `getline()` may slurp the whole (small) file
into `stdin`'s buffer in one `read()`. Every forked built-in inherits a
*copy* of that buffer via `fork()`. When such a child calls `exit()`,
libc notices unread bytes still sitting in its copy of the buffer and
`lseek()`s the underlying fd backward to "return" them — but that fd's
file offset is *shared* with the parent (fork duplicates the descriptor,
not the underlying open file description). Enough children doing this
independently walks the shared offset back past the start of the file,
and the parent starts re-reading — and re-executing — the script from
the top, forever. `_exit()` skips stdio cleanup entirely, so children
flush only their own stdout/stderr by hand and exit via the raw syscall.
A sibling bug hit `stdout` for the same reason (buffered output
duplicated across `fork()`); the fix there is line-buffering stdout via
`setvbuf` plus an explicit `fflush()` immediately before every `fork()`.

**Why not use `valgrind`?** Not available in this sandbox (no root to
install it). Verified instead with GCC's built-in AddressSanitizer +
LeakSanitizer + UndefinedBehaviorSanitizer
(`-fsanitize=address,leak,undefined`), which catches the same class of
bugs and doesn't need a package install — run against the full test
suite plus targeted scripts for pipes, redirection, background jobs, and
parser error paths. Zero leaks, zero errors.

## Concepts Demonstrated

| Feature | Syscalls | Why |
|---|---|---|
| External commands | `fork`, `execve`, `waitpid` | Child replaces its image; parent blocks until it exits |
| Pipes | `pipe`, `fork` ×N, `dup2`, `close` | Every process closes every fd it doesn't use — hold the write end open anywhere unintended and the reader never sees EOF |
| Redirection | `open`, `dup2`, `close` | Replaces the child's stdin/stdout fd before `execve`/builtin dispatch |
| Background jobs | `fork`, `setpgid`, `SIGCHLD` | No blocking `waitpid`; reaped asynchronously via a signal-set flag |
| `SIGINT`/`SIGTSTP` | `sigaction` | Parent ignores them (a shell shouldn't die on Ctrl+C); child resets to `SIG_DFL` so it can still be killed |
| `SIGCHLD` | `sigaction`, `waitpid(WNOHANG)` | Handler only sets a `volatile sig_atomic_t` flag — no `malloc`/`printf` in a signal handler, since neither is async-signal-safe. Actual reaping happens in the main loop |
| `cd` | `chdir` | Must run in the parent — a forked `cd` would only change *its own* cwd |
| `pwd` | `getcwd(NULL, 0)` | Dynamically sized, caller frees |
| `which` | `access(path, X_OK)` | Walks `PATH`, checks execute permission without actually running anything |

## Known Limitations

- `$VAR` expansion only triggers when a token *starts* with `$` (no
  embedded substitution like `foo$VARbar` or `foo${VAR}bar`)
- No `fg`/`bg`/`jobs` builtins — background jobs are launched and reaped
  automatically, but there's no interactive job control (that needs
  `tcsetpgrp` terminal ownership transfer, intentionally out of scope)
- No command substitution (`` `cmd` `` / `$(cmd)`) or globbing (`*.c`)

## Author

**OmegaZeroG** — [github.com/OmegaZeroG](https://github.com/OmegaZeroG)
