# My Shell 

**A POSIX-style Unix shell, written from scratch in C — no `<string.h>`, no
shell libraries. Just `fork`/`execve`/`pipe`/`dup2`/`sigaction` and a
hand-written parser.**

[![CI](https://github.com/OmegaZeroG/My_Shell/actions/workflows/ci.yml/badge.svg)](https://github.com/OmegaZeroG/My_Shell/actions/workflows/ci.yml)
![Language](https://img.shields.io/badge/language-C-00599C.svg)
![Warnings](https://img.shields.io/badge/warnings-0%20(-Wall%20-Wextra%20-Wshadow%20-Werror)-success.svg)
![Leaks](https://img.shields.io/badge/leaks-0%20(ASan%2FLSan)-success.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20WSL-informational.svg)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

It supports N-stage pipelines, I/O redirection, `&&`/`||`/`;` chaining with
proper exit-status short-circuiting, background jobs, `$VAR`/`$?` expansion,
quoted arguments, and the usual built-ins — all built on top of a custom
string library, because `<string.h>` was off the table from day one.

## At a glance

| | |
|---|---|
| **Language** | C (GNU17, `gcc`) |
| **Source** | 9 `.c` files, 1 header, ~1,500 lines — zero third-party dependencies |
| **Tests** | 24 regression cases, PASS/FAIL, CI-gated |
| **Compiler warnings** | 0, with `-Wall -Wextra -Wshadow -Werror` |
| **Memory leaks** | 0, verified with AddressSanitizer + LeakSanitizer + UBSan |
| **String library** | 100% hand-rolled — no `<string.h>` anywhere |

## Table of Contents

- [Demo](#demo)
- [Features](#features)
- [Architecture](#architecture)
- [Built-in Commands](#built-in-commands)
- [Getting Started](#getting-started)
- [Project Structure](#project-structure)
- [Custom String Library](#custom-string-library)
- [Design Decisions](#design-decisions)
- [Concepts Demonstrated](#concepts-demonstrated)
- [Known Limitations](#known-limitations)
- [Roadmap](#roadmap)
- [License](#license)

## Demo

```
$ ./my_shell
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

[my_shell]$ false
[my_shell]$ echo $?
1

[my_shell]$ sleep 3 &
[bg] launched pid 4821 (sleep)
[my_shell]$ echo still-responsive
still-responsive
[bg] pid 4821 done: sleep

[my_shell]$ echo "quoted args" foo"glued to this"bar | cat
quoted args fooglued to thisbar

[my_shell]$ exit
```

It's also fully scriptable — no TTY required:

```bash
./my_shell < script.sh          # run a script of shell commands
printf 'pwd\nls | wc -l\n' | ./my_shell   # or pipe commands straight in
```

## Features

**Parsing**
- Pipelines of any length: `cmd1 | cmd2 | cmd3 | ...`
- I/O redirection: `<`, `>`, `>>`
- Command chaining: `;`, `&&`, `||` — with real short-circuit evaluation
  based on the previous command's exit status, not just "run everything"
- Quoted arguments: `"like this"` or `'like this'`, including glued to
  unquoted text (`foo"bar baz"qux` → `foobar bazqux`)
- Variable expansion: `$VAR`, `$?` (last exit status), resolved at
  *execution* time so `$?` always reflects the pipeline that just ran
- Syntax-error detection for malformed input (leading/trailing `|`,
  redirection with no filename, etc.) instead of silently misbehaving

**Execution**
- N-way pipes via `pipe()`/`fork()`/`dup2()`, not just single-stage
- PATH resolution for external commands, with a fast path for `./foo` and
  absolute paths
- Built-ins (`echo`, `pwd`, `env`, `which`) run through the *same*
  fork/pipe/redirect machinery as external commands, so `pwd > out.txt` or
  `env | grep PATH` work exactly as you'd expect
- Background jobs: `sleep 10 &` returns to the prompt immediately, reaped
  and reported via `SIGCHLD`
- Signal handling: the shell itself survives `Ctrl+C`/`Ctrl+Z`; a
  foreground child still dies/suspends normally

**Usability**
- Arrow-key (↑↓) command history navigation via raw-mode terminal input
- Scriptable: falls back to plain `getline()` reads when stdin isn't a
  TTY, so `./my_shell < script.sh` and `cmd | ./my_shell` both just work

**Engineering**
- Zero compiler warnings under `-Wall -Wextra -Wshadow -Werror`
- Zero memory leaks, verified with AddressSanitizer + LeakSanitizer
- 24-case regression suite, run in CI on every push
- No `<string.h>` — every string operation is hand-written

## Architecture

```
  raw input line
        │
        ▼
  tokenizer          quotes ("...", '...'), operators (| < > >> && || ; &)
        │
        ▼
  parser             builds Command { argv, infile, outfile, append }
        │             grouped into Pipeline*[] chained by && / || / ;
        ▼
  shell_loop dispatch (main.c)
        │             tracks $?, short-circuits on && / ||
        ▼
  run_pipeline()
        │
        ├── standalone cd / setenv / unsetenv / exit ──▶ runs in the PARENT, no fork
        │                                                 (state must survive the command)
        └── everything else
                  │
                  ▼
            execute_pipeline_chain()
                  │             fork ×N, pipe ×(N-1), dup2, close
                  │
        ┌─────────┴─────────┐
        ▼                   ▼
  builtin in child     execve() external
  echo/pwd/env/which    PATH-resolved command
```

Every command — builtin or external — that isn't one of the state-mutating
special forms goes through the exact same fork/pipe/redirect path. That's
what makes `pwd > out.txt` and `env | grep PATH` behave correctly without
any special-casing in the pipe/redirection logic itself.

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
when run standalone; used inside a pipe (`cd /tmp | echo hi`) they run in a
throwaway subshell whose side effects don't survive the pipe — same as bash.

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
exits non-zero if anything fails — the same suite runs in
[CI](.github/workflows/ci.yml) on every push, once against the plain
release build and once against an AddressSanitizer/LeakSanitizer/UBSan
build.

## Project Structure

```
.
├── main.c                  # Shell loop: reads input, dispatches pipelines, tracks $?
├── input_parser.c          # Tokenizer + parser (quotes, pipes, redirection, &&/||/;/&) + $VAR expansion
├── executor.c               # fork/pipe/dup2 execution, PATH resolution, redirection
├── builtins.c                # cd, pwd, echo, env, which, setenv, unsetenv
├── signals.c                  # SIGINT/SIGTSTP/SIGCHLD setup for parent & child
├── jobs.c                      # Background job tracking + SIGCHLD-driven reaping
├── history.c                    # Raw-mode input reader, arrow-key history navigation
├── helpers.c                     # Custom string library (my_strlen, my_strcmp, etc.)
├── my_shell.h                     # Shared structs (Command, Pipeline) and declarations
├── tests/test.sh                   # Regression suite (24 cases, CI-gated)
├── .github/workflows/ci.yml         # Build + test + sanitizer CI
└── Makefile
```

## Custom String Library

Built without `<string.h>`. Every string operation is hand-rolled in
`helpers.c`:

| Function | Equivalent | Notes |
|---|---|---|
| `my_strlen` | `strlen` | |
| `my_strcmp` / `my_strncmp` | `strcmp` / `strncmp` | |
| `my_strdup` | `strdup` | Heap-allocates, caller frees |
| `my_strchr` | `strchr` | |
| `my_strcpy` | `strncpy`-ish | Bounded, zero-pads the remainder |
| `my_strtok` | `strtok` | Used for splitting `PATH` on `:` |
| `my_getenv` | `getenv` | Walks a `char**` env array directly (no global environ dependency) |
| `count_env_vars` | — | Counts entries in a `NULL`-terminated `char**` |

## Design Decisions

**Why a `Pipeline`/`Command` split, not a flat argv?** A line like
`a | b > out.txt && c` needs to represent: multiple commands chained by
pipes, each with its own optional redirection, connected to sibling
pipelines by `&&`/`||`/`;`. `Command` owns one program's argv plus its
`<`/`>`/`>>` targets; `Pipeline` owns an ordered list of `Command`s plus how
it connects to the *next* pipeline on the line. Parsing produces an array of
`Pipeline*`, and `main.c`'s dispatch loop walks it left to right,
short-circuiting on `&&`/`||` based on the previous exit status — the same
evaluation order a real shell uses.

**Why do `echo`/`pwd`/`env`/`which` fork instead of running inline?** So
they compose with pipes and redirection like any other command. If `pwd`
ran directly in the parent, `pwd > out.txt` would have to `dup2` the
shell's *own* stdout — permanently redirecting every future prompt.
Forking isolates the redirection to that one command, exactly like a real
external program. `cd`/`setenv`/`unsetenv`/`exit` are the exception: their
entire purpose is to mutate parent-shell state, so they only run
un-forked, and only when they're the sole command with no redirection.

**Why `_exit()` instead of `exit()` in a forked built-in?** This one cost
real debugging time. `exit()` flushes *and closes* every open stdio
stream, including `stdin`. If the shell reads its script from a file
(`./my_shell < script.sh`), `getline()` may slurp the whole (small) file
into `stdin`'s buffer in a single `read()`. Every forked built-in inherits
a *copy* of that buffer via `fork()`. When such a child calls `exit()`,
libc notices unread bytes still sitting in its copy of the buffer and
`lseek()`s the underlying fd backward to "return" them — but that fd's
file offset is *shared* with the parent (`fork()` duplicates the
descriptor, not the underlying open file description). Enough children
doing this independently walks the shared offset back past the start of
the file, and the parent starts re-reading — and re-executing — the
script from the top, forever. `_exit()` skips stdio cleanup entirely, so
children flush only their own stdout/stderr by hand and exit via the raw
syscall. A sibling bug hit `stdout` for the same reason (buffered output
duplicated across `fork()`); the fix there is line-buffering stdout via
`setvbuf` plus an explicit `fflush()` immediately before every `fork()`.

**Why does `$?` get expanded at execution time, not parse time?** A line
like `false; echo $?` parses both pipelines *before* either one runs, so if
expansion happened during parsing, `$?` would resolve against whatever the
*previous line's* status was — wrong. Instead, `expand_pipeline()` runs
immediately before each individual pipeline executes, so `$?` always
reflects the pipeline that just finished, even within a single input line.

**Why not use `valgrind`?** Not available in the environment this was
built and iterated in (no root to install it). Verified instead with
GCC's built-in AddressSanitizer + LeakSanitizer + UndefinedBehaviorSanitizer
(`-fsanitize=address,leak,undefined`), which catches the same class of
bugs and needs no package install — run against the full test suite plus
targeted scripts for pipes, redirection, background jobs, and parser
error paths, and wired into CI so it stays clean going forward.

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
| Non-tty input | `isatty`, `getline` | Detects piped/redirected stdin and falls back from raw-mode terminal reads to plain line reads |

## Known Limitations

These are deliberate scope cuts, not oversights:

- `$VAR` expansion only triggers when a token *starts* with `$` (no
  embedded substitution like `foo$VARbar` or `foo${VAR}bar`)
- No `fg`/`bg`/`jobs` builtins — background jobs are launched and reaped
  automatically, but there's no interactive job control (that needs
  `tcsetpgrp` terminal-ownership transfer, intentionally out of scope)
- No command substitution (`` `cmd` `` / `$(cmd)`) or globbing (`*.c`)

## Roadmap

- [ ] `fg`/`bg`/`jobs` builtins with `tcsetpgrp` terminal control
- [ ] Command substitution: `` `cmd` `` and `$(cmd)`
- [ ] Globbing: `*.c`, `?`, `[abc]`
- [ ] `${VAR}` / embedded expansion (`foo${VAR}bar`)
- [ ] `.myshrc` startup file support

## License

[MIT](LICENSE) — see the LICENSE file for details.

## Author

**Om Pathrabe** ([OmegaZeroG](https://github.com/OmegaZeroG))
