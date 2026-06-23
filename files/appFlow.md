# AppFlow — My_Shell
**How the shell processes every input from keypress to process exit**

---

## 1. Top-Level Shell Loop

```
main()
└── shell_loop(env)
    ├── setup signals (SIGINT=IGN, SIGCHLD=handler)
    ├── getcwd → initial_directory
    ├── history_init()
    └── LOOP:
        ├── print prompt "[my_shell]$ "
        ├── read_input()          → raw string (handles arrow keys, backspace, Ctrl+D)
        ├── if NULL (Ctrl+D)      → break → exit
        ├── if empty string       → free, continue
        ├── history_add()
        ├── parse_input()         → Pipeline struct
        ├── dispatch(pipeline)    → route to builtin or executor
        ├── free_pipeline()
        └── repeat
```

---

## 2. Input Parsing Flow

```
raw string: "ls -la | wc -l > out.txt"

parse_input()
├── tokenise()
│   ├── split on: spaces, |, <, >, >>, &, &&, ||
│   ├── quote handling: "hello world" → single token
│   └── $VAR expansion: $HOME → getenv value
│
├── build Command structs
│   ├── cmd[0]: argv=["ls","-la"], infile=NULL, outfile=NULL
│   └── cmd[1]: argv=["wc","-l"], infile=NULL, outfile="out.txt", append=0
│
└── return Pipeline { cmds=[cmd0,cmd1], count=2 }
```

---

## 3. Dispatch Flow

```
dispatch(pipeline)
├── single command, no pipe?
│   ├── is builtin? (cd, pwd, echo, env, setenv, unsetenv, which, jobs, fg, bg, exit)
│   │   └── call builtin directly in parent process
│   └── not builtin?
│       └── executor(cmd, env)
│
└── multiple commands (pipe chain)?
    └── execute_pipeline(pipeline, env)
```

**Why builtins run in parent:** `cd` must change the shell's own working directory. If it ran in a child, `chdir()` would affect only the child — the shell's cwd wouldn't change. Same logic for `setenv`/`unsetenv` (must modify shell's own `env` pointer).

---

## 4. External Command Execution

```
executor(cmd, env)
├── fork()
│   ├── CHILD:
│   │   ├── reset signals (SIGINT=DFL, SIGCHLD=DFL)
│   │   ├── apply_redirections(cmd)   [if any < > >>]
│   │   ├── child_process(cmd, env)
│   │   │   ├── get_path(env)          → PATH string
│   │   │   ├── split_paths(PATH)      → path_list[]
│   │   │   ├── for each path:
│   │   │   │   ├── build full_path = path + "/" + cmd
│   │   │   │   └── execve(full_path, argv, env)  → replaces process if found
│   │   │   ├── free path_list (all entries + array)
│   │   │   └── exit(127) "command not found"
│   │   └── [never returns if execve succeeds]
│   │
│   └── PARENT:
│       ├── background? → add to job table, continue loop
│       └── foreground? → waitpid(child_pid, &status, 0)
│           ├── WIFEXITED   → store WEXITSTATUS in $?
│           └── WIFSIGNALED → print "killed by signal N"
```

---

## 5. I/O Redirection Flow

```
apply_redirections(cmd)   [runs in child, before execve]

cmd->infile set?
    fd = open(infile, O_RDONLY)
    dup2(fd, STDIN_FILENO)    → stdin now reads from file
    close(fd)                  → close original fd

cmd->outfile set + append=0?
    fd = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0644)
    dup2(fd, STDOUT_FILENO)
    close(fd)

cmd->outfile set + append=1?
    fd = open(outfile, O_WRONLY|O_CREAT|O_APPEND, 0644)
    dup2(fd, STDOUT_FILENO)
    close(fd)
```

`dup2(fd, STDOUT_FILENO)` makes file descriptor 1 (stdout) point to the file. `execve`'d program inherits this — it writes to file without knowing.

---

## 6. Pipe Execution Flow

```
execute_pipeline([cmd1, cmd2], env)

pipe(pipefd)    → pipefd[0]=read, pipefd[1]=write

fork → child1 (cmd1):
    dup2(pipefd[1], STDOUT_FILENO)  → cmd1's stdout → pipe
    close(pipefd[0])                 → child1 never reads from pipe
    close(pipefd[1])                 → original fd closed after dup
    apply_redirections(cmd1)         → handle any < on cmd1
    execve(cmd1)

fork → child2 (cmd2):
    dup2(pipefd[0], STDIN_FILENO)   → cmd2's stdin ← pipe
    close(pipefd[1])                 → CRITICAL: close write end
    close(pipefd[0])                 → original fd closed after dup
    apply_redirections(cmd2)         → handle any > on cmd2
    execve(cmd2)

parent:
    close(pipefd[0])                 → CRITICAL
    close(pipefd[1])                 → CRITICAL: child2 gets EOF when done
    waitpid(child1, ...)
    waitpid(child2, ...)
```

---

## 7. Signal Flow

```
User presses Ctrl+C (SIGINT)
├── Shell (parent): SIG_IGN → nothing happens to shell
└── Foreground child: SIG_DFL → child receives SIGINT → terminates
    └── parent's waitpid returns with WIFSIGNALED=true

User presses Ctrl+Z (SIGTSTP)
├── Shell: SIG_IGN
└── Foreground child: SIG_DFL → child suspended
    └── shell adds to job table as JOB_STOPPED

Background child exits (SIGCHLD)
└── Shell SIGCHLD handler:
    └── waitpid(-1, &status, WNOHANG) loop
        └── update job table entry → JOB_DONE
        └── print "[job_id]+ Done  command"
```

---

## 8. Background Job Flow

```
"sleep 10 &"

parse → cmd.background = 1

executor:
    fork → child:
        setpgid(0, 0)         → own process group (Ctrl+C won't kill it)
        execve("sleep", ...)
    parent (no waitpid):
        job_add(child_pid, "sleep 10")
        print "[1] 12345"
        continue loop immediately

"jobs" command:
    print all job table entries with status

"fg 1" command:
    kill(job.pid, SIGCONT)    → resume if stopped
    tcsetpgrp(STDIN, job.pgid) → give terminal to job
    waitpid(job.pid, ...)      → wait for it
    tcsetpgrp(STDIN, shell_pgid) → take terminal back

"bg 1" command:
    kill(job.pid, SIGCONT)    → resume in background
    job.status = JOB_RUNNING
```

---

## 9. Exit / Cleanup Flow

```
Ctrl+D or "exit" command
└── shell_loop():
    ├── history_free(&hist)
    ├── free(initial_directory)
    ├── if env_is_heap: free(env)
    └── return to main()
        └── return 0
```
