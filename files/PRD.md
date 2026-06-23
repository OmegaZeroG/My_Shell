# PRD — My_Shell
**Product Requirements Document**
Owner: OmegaZeroG | Target: SDE Fresher Interview | Domain: OS / Systems

---

## 1. Purpose

Build a Unix shell in C from scratch that demonstrates mastery of core OS concepts — process management, inter-process communication, file descriptors, signal handling, and memory management — making it a defensible, interview-worthy systems project.

---

## 2. Goals

| Goal | Description |
|---|---|
| Interview readiness | Every feature maps to an OS interview question |
| Zero defects | Valgrind-clean, no leaks, no undefined behaviour |
| Code quality | No dead code, consistent error handling, readable |
| Completeness | No "coming soon" stubs in README or code |
| Depth over breadth | Fewer features done correctly > many features done badly |

---

## 3. Non-Goals

- GUI or TUI interface
- Full POSIX compliance
- Windows support
- Script language features (functions, loops, conditionals)
- Network or file-system utilities beyond shell scope

---

## 4. Users

Single user: the developer (you). Secondary audience: interviewers reading the GitHub repo.

---

## 5. Features — Prioritised

### P0 — Must ship (project is broken without these)

| ID | Feature | Why it matters |
|---|---|---|
| F01 | Fix memory leaks (path_list, env) | Valgrind errors kill credibility |
| F02 | Fix VLA in split_paths → malloc | Stack overflow risk on long PATH |
| F03 | Remove dead/commented code | Interviewers read code; stray `//printf("BYII")` looks amateur |
| F04 | Implement `which` | Listed in README, not implemented — embarrassing |
| F05 | Fix shadowed `input` variable | Undefined behaviour, shadow warning |

### P1 — Core OS features (project is incomplete without these)

| ID | Feature | OS concept demonstrated |
|---|---|---|
| F06 | Signal handling (SIGINT, SIGCHLD) | Signal propagation, process groups |
| F07 | I/O redirection (`>`, `<`, `>>`) | `open()`, `dup2()`, file descriptors |
| F08 | Pipes — single (`cmd1 \| cmd2`) | `pipe()`, `dup2()`, IPC, fd discipline |
| F09 | Exit status (`$?`) + `&&` / `\|\|` | `WEXITSTATUS()`, error propagation |

### P2 — Strong differentiators

| ID | Feature | OS concept demonstrated |
|---|---|---|
| F10 | Background jobs (`cmd &`) + `fg`/`bg`/`jobs` | `SIGCHLD`, `waitpid(WNOHANG)`, job table |
| F11 | Pipe chains (`a \| b \| c`) | N-1 pipes, fd close discipline, deadlock |
| F12 | Full `$VAR` expansion (`$HOME`, `$PWD`, `$?`) | Env lookup, expansion order |
| F13 | Quote handling (`"hello world"` as one token) | Parser state machine |

### P3 — Bonus (rare for freshers)

| ID | Feature | OS concept demonstrated |
|---|---|---|
| F14 | Semicolon chaining (`cmd1; cmd2`) | Sequential exec, exit code chain |
| F15 | Wildcards/globbing (`ls *.c`) | `opendir()`, `readdir()`, `fnmatch()` |
| F16 | Script mode (`./my_shell script.sh`) | Non-interactive mode, argv parsing |

---

## 6. Success Criteria

- `valgrind --leak-check=full ./my_shell` → zero errors, zero leaks
- All P0 + P1 features implemented and working
- Test suite passes 100%
- Interviewer can ask any OS question (fork, exec, pipe, dup2, signal, fd) and answer can be demonstrated live in the shell
- README contains no "coming soon" items

---

## 7. Resume Bullet (target)

> Built Unix shell in C from scratch: `fork/execve` process model, `pipe()`-based IPC, `dup2()` I/O redirection, signal handling (SIGINT/SIGCHLD), job control, and env management — zero memory leaks verified under Valgrind.
