# implementationPlan.md — My_Shell
**Step-by-step build order with exact tasks per phase**

---

## Phase 0 — Cleanup (do first, ~2 hours)
*Goal: clean codebase before adding features*

### 0.1 Dead code removal (`main.c`)
- [ ] Remove block comment at top of `shell_buildins` (the 8-line comment list)
- [ ] Remove `//printf("BYII")` in exit branch
- [ ] Remove commented `for` loop that prints ARGS
- [ ] Remove `char* input = NULL` and `size_t input_size = 0` at top of `shell_loop` (shadowed/unused)
- [ ] Remove `(void)argc` and `(void)argv` if argc/argv truly unused — or use them for script mode later

### 0.2 Dead code removal (`builtins.c`)
- [ ] Remove `// (void)env;` in `command_echo`
- [ ] Remove `// printf("\n");` in `command_env`

### 0.3 Fix memory leaks (`executor.c`)
- [ ] `split_paths`: replace VLA with `malloc` + `free(paths_copy)` at end
- [ ] `child_process`: add `free_paths(path_list, num_paths)` before `exit(EXIT_FAILURE)` on command not found
- [ ] Add `free_paths(char** list, int count)` helper in `executor.c`

### 0.4 Fix env leak (`main.c`)
- [ ] Add `int env_is_heap = 0` in `shell_loop`
- [ ] Wrap `setenv`/`unsetenv` calls: save old pointer, check flag, free if heap, update flag

### 0.5 Consistent error handling (all files)
- [ ] Audit every error output — change any `printf(stderr...)` to `fprintf(stderr, ...)`
- [ ] Ensure every syscall failure uses `perror()`
- [ ] Ensure every logic error uses `fprintf(stderr, "my_shell: ...")`

### 0.6 Implement `which` (`builtins.c`)
- [ ] Remove declaration stub `int command_which(char** args, char** env);`
- [ ] Write full implementation using `get_path`, `split_paths`, `access(X_OK)`, `free_paths`
- [ ] Uncomment `command_which(args, env)` call in `main.c`
- [ ] Add `#include <unistd.h>` to `my_shell.h` if missing

### 0.7 Update Makefile
- [ ] Add `-Wshadow` flag
- [ ] Add `-Werror` flag
- [ ] Confirm build is warning-free after above fixes

### 0.8 Verify
- [ ] `make` → zero warnings, zero errors
- [ ] `valgrind --leak-check=full --track-origins=yes ./my_shell` → test basic commands → zero errors

---

## Phase 1 — Signals (~2-3 hours)
*Goal: shell survives Ctrl+C, background children are reaped*

### 1.1 Create `signals.c` and `signals.h`
- [ ] `setup_signals()`: set SIGINT=SIG_IGN, SIGTSTP=SIG_IGN, SIGCHLD=sigchld_handler in parent
- [ ] `sigchld_handler()`: loop `waitpid(-1, &status, WNOHANG)`, update global job table
- [ ] Use `sigaction()` not `signal()` — more reliable, portable

### 1.2 Call setup in `shell_loop`
- [ ] Call `setup_signals()` at start of `shell_loop`

### 1.3 Reset signals in child
- [ ] In `child_process()` (or new `reset_signals()`), before `execve`: set SIGINT=DFL, SIGCHLD=DFL, SIGTSTP=DFL

### 1.4 Verify
- [ ] Run shell, press Ctrl+C → shell prompt returns, shell doesn't die
- [ ] Run `sleep 5`, press Ctrl+C → sleep dies, shell prompt returns

---

## Phase 2 — I/O Redirection (~4 hours)
*Goal: `>`, `<`, `>>` work correctly*

### 2.1 Update parser (`input_parser.c`)
- [ ] Tokenise `>`, `<`, `>>` as distinct token types
- [ ] Fill `Command.outfile`, `Command.infile`, `Command.append` during parse
- [ ] Ensure redirect tokens + filenames are NOT included in `argv`

### 2.2 Create `apply_redirections(Command* cmd)` in `executor.c`
- [ ] If `infile`: `open(O_RDONLY)`, `dup2(fd, STDIN_FILENO)`, `close(fd)`
- [ ] If `outfile` + `append=0`: `open(O_WRONLY|O_CREAT|O_TRUNC, 0644)`, `dup2(fd, STDOUT_FILENO)`, `close(fd)`
- [ ] If `outfile` + `append=1`: `open(O_WRONLY|O_CREAT|O_APPEND, 0644)`, `dup2(fd, STDOUT_FILENO)`, `close(fd)`
- [ ] On `open` failure: `perror`, `exit(EXIT_FAILURE)` from child

### 2.3 Call `apply_redirections` in child
- [ ] Call after `fork`, before `execve`, in `child_process`

### 2.4 Verify
- [ ] `echo hello > test.txt` → file created with "hello"
- [ ] `echo world >> test.txt` → file has "hello\nworld"
- [ ] `cat < test.txt` → prints file content
- [ ] `ls -la > /dev/null` → no output, no error

---

## Phase 3 — Pipes single (~1 day)
*Goal: `cmd1 | cmd2` works*

### 3.1 Update parser
- [ ] Tokenise `|` as TOKEN_PIPE
- [ ] `parse_input` returns `Pipeline` with multiple `Command*` when pipe found

### 3.2 Update `my_shell.h` with structs
- [ ] Add `Pipeline` struct (from schema.md)
- [ ] Add `Command` struct with `infile`, `outfile`, `append`, `background`

### 3.3 Create `execute_pipeline(Pipeline* p, char** env)` in `executor.c`
- [ ] If count==1: call existing `executor(cmd, env)` (no pipe needed)
- [ ] If count==2:
  - [ ] `pipe(pipefd)`
  - [ ] `fork` child1: `dup2(pipefd[1], STDOUT)`, close both raw ends, `execve(cmd[0])`
  - [ ] `fork` child2: `dup2(pipefd[0], STDIN)`, close both raw ends, `execve(cmd[1])`
  - [ ] Parent: close `pipefd[0]` and `pipefd[1]`, `waitpid` both children

### 3.4 Verify
- [ ] `ls | wc -l` → correct line count
- [ ] `echo hello | cat` → prints "hello"
- [ ] `cat /dev/null | wc -l` → "0"
- [ ] `ls nonexistent 2>/dev/null | wc -l` → "0" (test error pipe)

---

## Phase 4 — Exit status + `&&` / `||` (~2 hours)
*Goal: `$?`, `&&`, `||` work*

### 4.1 Track last exit status
- [ ] Add `int last_exit_status = 0` in `shell_loop`
- [ ] After every `waitpid`: `last_exit_status = WIFEXITED(s) ? WEXITSTATUS(s) : 1`
- [ ] Pass `last_exit_status` through dispatch chain

### 4.2 `$?` expansion in parser
- [ ] In `expand_variables()`: if token == `"$?"` → return `itoa(last_exit_status)`

### 4.3 `&&` / `||` in parser + dispatcher
- [ ] Tokenise `&&` → TOKEN_AND, `||` → TOKEN_OR
- [ ] In `shell_loop` dispatch: after running pipeline, check connector:
  - `&&` → run next only if `last_exit_status == 0`
  - `||` → run next only if `last_exit_status != 0`

### 4.4 Verify
- [ ] `ls /tmp && echo ok` → prints "ok"
- [ ] `ls /nonexistent && echo ok` → doesn't print "ok"
- [ ] `ls /nonexistent || echo fallback` → prints "fallback"
- [ ] `echo $?` after failed command → prints non-zero

---

## Phase 5 — Background jobs (~1 day)
*Goal: `cmd &`, `jobs`, `fg`, `bg` work*

### 5.1 Create `jobs.c` and `jobs.h`
- [ ] Implement `jobs_init`, `job_add`, `job_find`, `job_remove`, `job_print`, `jobs_check_done`
- [ ] Initialise `JobTable` in `shell_loop`

### 5.2 Background execution in `executor.c`
- [ ] If `cmd->background`: after fork, `setpgid(child, child)` in child, no `waitpid` in parent
- [ ] Call `job_add(jt, pid, pid, cmd_string)`
- [ ] Print `[job_id] pid`

### 5.3 SIGCHLD handler update
- [ ] On child exit: find job by pid, set `JOB_DONE`, store exit code
- [ ] Print `[job_id]+ Done  command` next prompt

### 5.4 Builtins: `jobs`, `fg`, `bg`
- [ ] `jobs`: print all non-DONE jobs
- [ ] `fg N`: `kill(job.pid, SIGCONT)`, `tcsetpgrp(STDIN, job.pgid)`, `waitpid`, restore shell pgid
- [ ] `bg N`: `kill(job.pid, SIGCONT)`, mark JOB_RUNNING

### 5.5 Verify
- [ ] `sleep 10 &` → `[1] pid` printed, prompt returns
- [ ] `jobs` → shows `[1]+ Running sleep 10`
- [ ] `fg 1` → waits for sleep
- [ ] `sleep 10 &` then Ctrl+C → sleep not killed

---

## Phase 6 — Pipe chains (~4 hours)
*Goal: `a | b | c` (3+ commands) works*

### 6.1 Update `execute_pipeline` for N commands
- [ ] Create N-1 pipes before forking
- [ ] Fork N children
- [ ] Child i: `dup2` read end of pipe[i-1] to stdin (if i>0), `dup2` write end of pipe[i] to stdout (if i<N-1)
- [ ] **Close ALL pipe ends in ALL children** (critical — loop over all pipes)
- [ ] Parent: close all pipe ends, `waitpid` all children

### 6.2 Verify
- [ ] `ls | sort | uniq | wc -l` → correct count
- [ ] `cat /dev/urandom | head -c 100 | wc -c` → "100"

---

## Phase 7 — Tests (~2 hours)
*Goal: automated regression suite*

### 7.1 Create `tests/test.sh`
- [ ] Run shell in non-interactive mode (script mode or pipe in commands)
- [ ] Test each feature: echo, pwd, cd, env, setenv, which, redirection, pipes, exit status, &&/||
- [ ] Assert expected output with `diff` or `grep`
- [ ] Print PASS/FAIL per test, count at end

### 7.2 Test cases minimum set
- [ ] echo hello world
- [ ] echo -n no newline
- [ ] echo $HOME
- [ ] pwd
- [ ] cd /tmp && pwd
- [ ] which ls
- [ ] ls > /tmp/out.txt && cat /tmp/out.txt
- [ ] echo hello >> /tmp/out.txt
- [ ] ls | wc -l
- [ ] ls | sort | uniq
- [ ] ls /nonexistent && echo bad || echo good
- [ ] echo $? after failure
- [ ] empty input (just Enter)
- [ ] nonexistent command
- [ ] setenv FOO bar && echo $FOO

---

## Phase 8 — README rewrite (~1 hour)

- [ ] Remove all "coming soon" items from Roadmap
- [ ] Add "Design Decisions" section: fork/execve rationale, custom string lib, env as char**
- [ ] Add "Concepts Demonstrated" section: list every syscall used
- [ ] Update Roadmap to reflect what's actually implemented
- [ ] Update build/run instructions if files changed
