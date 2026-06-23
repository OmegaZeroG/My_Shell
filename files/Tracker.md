# Tracker.md — My_Shell
**Live progress tracker — check off as you go**

---

## Phase 0 — Cleanup
| # | Task | File | Status |
|---|---|---|---|
| 0.1 | Remove block comment at top of `shell_buildins` | main.c | ⬜ |
| 0.2 | Remove `//printf("BYII")` | main.c | ⬜ |
| 0.3 | Remove commented ARGS `for` loop | main.c | ⬜ |
| 0.4 | Remove shadowed `char* input = NULL` | main.c | ⬜ |
| 0.5 | Remove unused `size_t input_size = 0` | main.c | ⬜ |
| 0.6 | Remove `// (void)env;` | builtins.c | ⬜ |
| 0.7 | Remove `// printf("\n");` | builtins.c | ⬜ |
| 0.8 | Replace VLA with malloc in `split_paths` | executor.c | ⬜ |
| 0.9 | Free `paths_copy` at end of `split_paths` | executor.c | ⬜ |
| 0.10 | Add `free_paths()` helper | executor.c | ⬜ |
| 0.11 | Call `free_paths` before `exit` on cmd not found | executor.c | ⬜ |
| 0.12 | Add `env_is_heap` flag in `shell_loop` | main.c | ⬜ |
| 0.13 | Free old env on `setenv` replacement | main.c | ⬜ |
| 0.14 | Free old env on `unsetenv` replacement | main.c | ⬜ |
| 0.15 | Audit + standardise all error output | all | ⬜ |
| 0.16 | Implement `command_which` | builtins.c | ⬜ |
| 0.17 | Uncomment `command_which` call | main.c | ⬜ |
| 0.18 | Add `-Wshadow -Werror` to Makefile | Makefile | ⬜ |
| 0.19 | `make` → zero warnings | — | ⬜ |
| 0.20 | Valgrind clean on basic commands | — | ⬜ |

---

## Phase 1 — Signals
| # | Task | File | Status |
|---|---|---|---|
| 1.1 | Create `signals.c` + `signals.h` | new | ⬜ |
| 1.2 | `setup_signals()`: SIGINT=IGN, SIGTSTP=IGN, SIGCHLD=handler | signals.c | ⬜ |
| 1.3 | `sigchld_handler()`: WNOHANG loop, update job table | signals.c | ⬜ |
| 1.4 | Call `setup_signals()` in `shell_loop` | main.c | ⬜ |
| 1.5 | Reset signals to DFL in child before `execve` | executor.c | ⬜ |
| 1.6 | Add `signals.c` to Makefile SRCS | Makefile | ⬜ |
| 1.7 | Test: Ctrl+C doesn't kill shell | — | ⬜ |
| 1.8 | Test: Ctrl+C kills foreground child | — | ⬜ |

---

## Phase 2 — I/O Redirection
| # | Task | File | Status |
|---|---|---|---|
| 2.1 | Tokenise `>`, `<`, `>>` in parser | input_parser.c | ⬜ |
| 2.2 | Fill `Command.infile`, `outfile`, `append` | input_parser.c | ⬜ |
| 2.3 | Exclude redirect tokens from `argv` | input_parser.c | ⬜ |
| 2.4 | Add `Command` struct with redirect fields to header | my_shell.h | ⬜ |
| 2.5 | Write `apply_redirections(Command* cmd)` | executor.c | ⬜ |
| 2.6 | Handle `<` (STDIN redir) | executor.c | ⬜ |
| 2.7 | Handle `>` (STDOUT truncate) | executor.c | ⬜ |
| 2.8 | Handle `>>` (STDOUT append) | executor.c | ⬜ |
| 2.9 | Call `apply_redirections` in child before exec | executor.c | ⬜ |
| 2.10 | Test: `echo hello > out.txt` | — | ⬜ |
| 2.11 | Test: `echo world >> out.txt` | — | ⬜ |
| 2.12 | Test: `cat < out.txt` | — | ⬜ |

---

## Phase 3 — Pipes (single)
| # | Task | File | Status |
|---|---|---|---|
| 3.1 | Tokenise `|` as TOKEN_PIPE | input_parser.c | ⬜ |
| 3.2 | Add `Pipeline` struct to header | my_shell.h | ⬜ |
| 3.3 | Parse multiple commands into `Pipeline` | input_parser.c | ⬜ |
| 3.4 | Write `execute_pipeline(Pipeline*, char**)` | executor.c | ⬜ |
| 3.5 | `pipe(pipefd)` call | executor.c | ⬜ |
| 3.6 | Fork child1: dup2 write end, close both, exec cmd[0] | executor.c | ⬜ |
| 3.7 | Fork child2: dup2 read end, close both, exec cmd[1] | executor.c | ⬜ |
| 3.8 | Parent: close both ends, waitpid both | executor.c | ⬜ |
| 3.9 | Wire `execute_pipeline` into dispatch | main.c | ⬜ |
| 3.10 | Test: `ls \| wc -l` | — | ⬜ |
| 3.11 | Test: `echo hello \| cat` | — | ⬜ |

---

## Phase 4 — Exit Status + `&&` / `||`
| # | Task | File | Status |
|---|---|---|---|
| 4.1 | Add `last_exit_status` to shell state | main.c | ⬜ |
| 4.2 | Store exit status after every waitpid | executor.c | ⬜ |
| 4.3 | `$?` expansion in `expand_variables` | input_parser.c | ⬜ |
| 4.4 | Tokenise `&&` and `\|\|` | input_parser.c | ⬜ |
| 4.5 | Implement `&&` dispatch logic | main.c | ⬜ |
| 4.6 | Implement `\|\|` dispatch logic | main.c | ⬜ |
| 4.7 | Test: `ls /tmp && echo ok` | — | ⬜ |
| 4.8 | Test: `ls /bad \|\| echo fallback` | — | ⬜ |
| 4.9 | Test: `echo $?` after failure | — | ⬜ |

---

## Phase 5 — Background Jobs
| # | Task | File | Status |
|---|---|---|---|
| 5.1 | Create `jobs.c` + `jobs.h` | new | ⬜ |
| 5.2 | Implement `jobs_init`, `job_add`, `job_find`, `job_remove` | jobs.c | ⬜ |
| 5.3 | Implement `jobs_check_done` (called from SIGCHLD handler) | jobs.c | ⬜ |
| 5.4 | Parse `&` as background flag | input_parser.c | ⬜ |
| 5.5 | Background fork: `setpgid(0,0)`, no waitpid, `job_add` | executor.c | ⬜ |
| 5.6 | Print `[N] pid` on background launch | executor.c | ⬜ |
| 5.7 | Implement `command_jobs` builtin | builtins.c | ⬜ |
| 5.8 | Implement `command_fg` builtin | builtins.c | ⬜ |
| 5.9 | Implement `command_bg` builtin | builtins.c | ⬜ |
| 5.10 | Test: `sleep 5 &` returns prompt | — | ⬜ |
| 5.11 | Test: `jobs` shows running job | — | ⬜ |
| 5.12 | Test: `fg 1` waits for sleep | — | ⬜ |
| 5.13 | Test: Ctrl+C doesn't kill background sleep | — | ⬜ |

---

## Phase 6 — Pipe Chains
| # | Task | File | Status |
|---|---|---|---|
| 6.1 | Update `execute_pipeline` for N>2 commands | executor.c | ⬜ |
| 6.2 | Create N-1 pipes before forking | executor.c | ⬜ |
| 6.3 | Fork N children, each closes all unused pipe ends | executor.c | ⬜ |
| 6.4 | Parent closes all N-1 pipe pairs | executor.c | ⬜ |
| 6.5 | Test: `ls \| sort \| uniq \| wc -l` | — | ⬜ |

---

## Phase 7 — Tests
| # | Task | File | Status |
|---|---|---|---|
| 7.1 | Create `tests/test.sh` | tests/ | ⬜ |
| 7.2 | Write 20+ test cases | tests/ | ⬜ |
| 7.3 | All tests pass | — | ⬜ |

---

## Phase 8 — README
| # | Task | Status |
|---|---|---|
| 8.1 | Remove all "coming soon" items | ⬜ |
| 8.2 | Add Design Decisions section | ⬜ |
| 8.3 | Add Concepts Demonstrated section with syscall list | ⬜ |
| 8.4 | Update roadmap to reflect done state | ⬜ |

---

## Summary Progress

| Phase | Total tasks | Done | % |
|---|---|---|---|
| 0 — Cleanup | 20 | 0 | 0% |
| 1 — Signals | 8 | 0 | 0% |
| 2 — Redirection | 12 | 0 | 0% |
| 3 — Pipes | 11 | 0 | 0% |
| 4 — Exit status | 9 | 0 | 0% |
| 5 — Background | 13 | 0 | 0% |
| 6 — Pipe chains | 5 | 0 | 0% |
| 7 — Tests | 3 | 0 | 0% |
| 8 — README | 4 | 0 | 0% |
| **Total** | **85** | **0** | **0%** |
