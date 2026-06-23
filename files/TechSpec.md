# TechSpec — My_Shell
**Technical Specification**
Owner: OmegaZeroG | Language: C (C99) | Platform: Linux / WSL

---

## 1. Architecture Overview

```
main.c              Shell loop, dispatcher
├── input_parser.c  Tokeniser, quote handling, expansion
├── executor.c      fork/execve, PATH resolution, pipes, redirection
├── builtins.c      cd, pwd, echo, env, setenv, unsetenv, which, jobs, fg, bg
├── history.c       Arrow-key history, circular buffer
├── signals.c       [NEW] SIGINT, SIGCHLD, SIGTSTP handlers
├── jobs.c          [NEW] Job table, background process tracking
└── helpers.c       Custom string lib (no <string.h>)
```

---

## 2. Data Structures

### 2.1 Environment (`char**`)
```c
char** env;   // NULL-terminated array of "KEY=VALUE" strings
              // heap-allocated after first setenv/unsetenv
int env_is_heap;  // flag: 0 = original argv env, 1 = malloc'd
```

### 2.2 Job Table
```c
typedef enum { JOB_RUNNING, JOB_STOPPED, JOB_DONE } job_status_t;

typedef struct {
    int     job_id;
    pid_t   pid;
    char    command[256];
    job_status_t status;
} Job;

typedef struct {
    Job   jobs[MAX_JOBS];   // MAX_JOBS = 64
    int   count;
} JobTable;
```

### 2.3 History
```c
typedef struct {
    char** entries;
    int    size;
    int    capacity;
    int    current;   // navigation index
} History;
```

### 2.4 Token / AST (minimal)
```c
typedef enum { TOKEN_WORD, TOKEN_PIPE, TOKEN_REDIR_IN,
               TOKEN_REDIR_OUT, TOKEN_REDIR_APPEND,
               TOKEN_BG, TOKEN_AND, TOKEN_OR } token_type_t;

typedef struct {
    char**  argv;       // NULL-terminated args
    char*   infile;     // input redirect target (NULL if none)
    char*   outfile;    // output redirect target (NULL if none)
    int     append;     // 1 if >>, 0 if >
    int     background; // 1 if &
} Command;

typedef struct {
    Command** cmds;   // pipe chain: cmds[0] | cmds[1] | ...
    int       count;
    int       and_or; // 0=none, 1=&&, 2=||
} Pipeline;
```

---

## 3. Syscall Map

| Feature | Syscalls used | Key behaviour |
|---|---|---|
| External command | `fork`, `execve`, `waitpid` | Child replaces image; parent waits |
| I/O redirection `>` | `open(O_WRONLY\|O_CREAT\|O_TRUNC)`, `dup2`, `close` | dup2 replaces stdout fd |
| I/O redirection `>>` | `open(O_WRONLY\|O_CREAT\|O_APPEND)`, `dup2`, `close` | appends to file |
| I/O redirection `<` | `open(O_RDONLY)`, `dup2`, `close` | replaces stdin fd |
| Pipe | `pipe`, `fork`×2, `dup2`, `close` | close unused ends before exec |
| Background job | `fork`, `setpgid`, no `waitpid` | child in own process group |
| Signal SIGINT | `signal(SIGINT, SIG_IGN)` in parent | child inherits SIG_DFL |
| Signal SIGCHLD | `sigaction`, `waitpid(WNOHANG)` | reap background children |
| Signal SIGTSTP | `kill(child_pid, SIGTSTP)` | suspend foreground job |
| `cd` | `chdir` | builtin — must run in parent process |
| `pwd` | `getcwd(NULL, 0)` | dynamic alloc, caller frees |
| `which` | `access(path, X_OK)` | PATH traversal |

---

## 4. Memory Rules

1. Every `malloc`/`my_strdup` has a matching `free` on every exit path.
2. `split_paths` — use `malloc` for path copy, NOT VLA.
3. `free_paths(char** list, int count)` helper — frees list entries + list itself.
4. `env` array: track `env_is_heap`. Free old array when replacing via setenv/unsetenv. Never free original `argv`-provided env.
5. `parse_input` returns heap-allocated array — caller calls `free_tokens()`.
6. Before `exit()` in child process — free all heap allocations (path_list, full_path).

---

## 5. Error Handling Standard

**One pattern everywhere — no mixing:**
```c
// External syscall failure:
perror("fork");    // prints "fork: <errno string>"
return -1;         // or exit(EXIT_FAILURE) in child

// Logic error (wrong args, not found):
fprintf(stderr, "my_shell: %s: command not found\n", cmd);
return 1;

// Fatal (can't continue):
perror("malloc");
exit(EXIT_FAILURE);
```

Never use `printf` for errors — always `fprintf(stderr, ...)` or `perror`.

---

## 6. Signal Architecture

```
Parent (shell loop)
├── SIGINT  → SIG_IGN  (shell ignores Ctrl+C)
├── SIGCHLD → handler: waitpid(WNOHANG) loop, update job table
└── SIGTSTP → SIG_IGN  (shell ignores Ctrl+Z)

Child (before execve)
├── SIGINT  → SIG_DFL  (reset to default — child can be killed)
├── SIGCHLD → SIG_DFL
└── SIGTSTP → SIG_DFL
```

Set child signals after `fork()`, before `execve()`:
```c
signal(SIGINT,  SIG_DFL);
signal(SIGCHLD, SIG_DFL);
signal(SIGTSTP, SIG_DFL);
```

---

## 7. Pipe Implementation (single)

```
pipe(pipefd)          → pipefd[0]=read end, pipefd[1]=write end

fork → child1 (left cmd):
    dup2(pipefd[1], STDOUT_FILENO)   // write to pipe
    close(pipefd[0])                  // CRITICAL: close unused read
    close(pipefd[1])                  // close after dup
    execve(...)

fork → child2 (right cmd):
    dup2(pipefd[0], STDIN_FILENO)    // read from pipe
    close(pipefd[1])                  // CRITICAL: close unused write
    close(pipefd[0])                  // close after dup
    execve(...)

parent:
    close(pipefd[0])                  // CRITICAL: parent closes both
    close(pipefd[1])
    waitpid(child1, ...)
    waitpid(child2, ...)
```

**Why close unused ends?** If parent holds write end open, child2 (reader) never sees EOF — blocks forever. This is deadlock. Every fd must be closed in every process that doesn't use it.

---

## 8. Build System

```makefile
CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -Wshadow -g
SRCS    = main.c executor.c builtins.c helpers.c \
          input_parser.c history.c signals.c jobs.c
TARGET  = my_shell
```

`-Wshadow` catches the `input` variable shadow bug class. `-Werror` means warnings = build failure. No shipping with warnings.

---

## 9. File Structure (final)

```
My_Shell/
├── main.c
├── executor.c
├── builtins.c
├── helpers.c
├── input_parser.c
├── history.c
├── signals.c        [NEW]
├── jobs.c           [NEW]
├── my_shell.h
├── Makefile
├── tests/
│   └── test.sh
└── README.md
```
