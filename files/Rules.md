# Rules.md — My_Shell
**Non-negotiable coding standards. Break these = code doesn't ship.**

---

## 1. Memory Rules

**R1.1** Every `malloc` / `my_strdup` / `realloc` has a matching `free` on EVERY exit path — normal return, error return, and `exit()` in child.

**R1.2** Never use VLAs (variable-length arrays on stack) with runtime-sized input. Use `malloc`. Stack overflows silently.

**R1.3** Never free memory you don't own. Original `argv`-provided `env` is owned by the OS. Track ownership with `env_is_heap`.

**R1.4** In a child process, free all heap allocations before calling `exit()`. Even though the OS will reclaim memory on process exit, clean child exits prove correctness and satisfy valgrind.

**R1.5** Add helper `free_paths(char** list, int count)` — reuse it everywhere path lists are freed. No copy-paste free loops.

**R1.6** Valgrind command to run after every phase:
```bash
valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./my_shell < tests/commands.txt
```
Zero errors. Zero leaks. Non-negotiable before marking any phase done.

---

## 2. Error Handling Rules

**R2.1** Syscall failures → `perror("syscall_name")`. Always. Never `printf`.

**R2.2** Logic/user errors → `fprintf(stderr, "my_shell: descriptive message\n")`.

**R2.3** Fatal errors (can't continue) → `perror(...)` then `exit(EXIT_FAILURE)`.

**R2.4** Never silently ignore errors. Every `malloc`, `fork`, `pipe`, `open`, `dup2`, `waitpid` return value is checked.

**R2.5** Child process: on `execve` failure, `free` heap allocations, then `exit(127)`. Exit code 127 = standard "command not found".

**R2.6** Never use `exit()` in the parent shell process except for `exit`/`quit` builtins. All other errors: print message, return to prompt.

---

## 3. File Descriptor Rules

**R3.1** After `pipe(pipefd)` and `fork`: every process closes every fd it doesn't use. No exceptions.

**R3.2** After `dup2(fd, STDIN_FILENO)`: close the original `fd`. Two fds pointing to same file = wasted descriptor.

**R3.3** Parent must close pipe ends after forking children. If parent holds write end open, reader child never sees EOF — deadlock.

**R3.4** Never pass raw pipe fds to `execve`'d programs (unless intentional). Close them first.

**R3.5** Check `dup2` return value. If it returns -1, `perror("dup2")` and `exit(EXIT_FAILURE)` from child.

---

## 4. Signal Rules

**R4.1** Parent shell: SIGINT = SIG_IGN, SIGTSTP = SIG_IGN. Shell must not die on Ctrl+C.

**R4.2** Child (after fork, before execve): reset SIGINT, SIGTSTP, SIGCHLD to SIG_DFL. Child must respond normally to signals.

**R4.3** SIGCHLD handler: use `waitpid(-1, &status, WNOHANG)` in a loop, not a single call. Multiple children can exit before one SIGCHLD is delivered.

**R4.4** Use `sigaction()` not `signal()`. `signal()` behaviour is implementation-defined on Linux (SA_RESTART vs not).

**R4.5** SIGCHLD handler only: update job table, print done notification. No malloc, no printf inside signal handler (not async-signal-safe). Use `write()` if you must print.

---

## 5. Code Quality Rules

**R5.1** Zero compiler warnings. Makefile must include `-Wall -Wextra -Werror -Wshadow`.

**R5.2** No commented-out code in committed code. If it's not active, delete it. Git history preserves old code.

**R5.3** No `printf` to stderr. All error output goes through `perror` or `fprintf(stderr, ...)`.

**R5.4** No magic numbers. Use named constants: `MAX_JOBS`, `MAX_ARGS`, `HISTORY_MAX`.

**R5.5** One function does one thing. If a function is over 50 lines, split it.

**R5.6** Every function that allocates has a matching free function. `parse_input` → `free_pipeline`. `split_paths` → `free_paths`.

**R5.7** Naming: `snake_case` for functions and variables. `UPPER_CASE` for macros and constants. No abbreviations that obscure meaning (`cmd` ok, `c` not ok for command).

---

## 6. Git Rules

**R6.1** One logical change per commit. "Fix memory leak in split_paths" not "stuff".

**R6.2** Commit message format: `<type>: <what>` — e.g. `fix: free path_list before exit in child_process` or `feat: add pipe support`.

**R6.3** Never commit with failing valgrind. If it leaks, don't push.

**R6.4** Never commit with compiler warnings. `-Werror` enforces this.

**R6.5** After each phase is fully working: tag it. `git tag phase0-cleanup`, `git tag phase1-signals`, etc.

---

## 7. Testing Rules

**R7.1** Every new feature gets a test in `tests/test.sh` before the feature is considered done.

**R7.2** Test covers: happy path, error case, edge case (empty input, non-existent file, etc.).

**R7.3** `tests/test.sh` exits 0 = all pass, exits 1 = any fail. CI-ready.

**R7.4** Run full test suite after every phase before moving to next.

---

## 8. Interview Readiness Rules

**R8.1** For every syscall used, you must be able to answer: what does it do, why is it used here, what happens if it fails, what's the alternative.

**R8.2** For every design decision, you must be able to answer: why this approach, what's the trade-off, what would you do differently at scale.

**R8.3** No "coming soon" or stub functions in README or code at interview time.

**R8.4** You must be able to draw the pipe execution flow (two forks, dup2s, fd closes) on a whiteboard from memory.

**R8.5** You must be able to answer "why is cd a builtin" without pausing.
