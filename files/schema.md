# schema.md — My_Shell
**All structs, enums, typedefs, and global state**

---

## 1. Token Types

```c
// input_parser.h
typedef enum {
    TOKEN_WORD,         // regular argument: "ls", "-la", "file.txt"
    TOKEN_PIPE,         // |
    TOKEN_REDIR_IN,     // <
    TOKEN_REDIR_OUT,    // >
    TOKEN_REDIR_APPEND, // >>
    TOKEN_BG,           // &  (background)
    TOKEN_AND,          // && (execute next only if prev succeeded)
    TOKEN_OR,           // || (execute next only if prev failed)
    TOKEN_SEMICOLON,    // ;  (execute next regardless)
    TOKEN_EOF
} token_type_t;

typedef struct {
    token_type_t type;
    char*        value;  // heap-allocated; NULL for non-WORD tokens
} Token;
```

---

## 2. Command

Represents a single command with its arguments and redirections.

```c
// my_shell.h
typedef struct {
    char**  argv;       // NULL-terminated arg list: ["ls", "-la", NULL]
    int     argc;       // count of args (not counting NULL terminator)
    char*   infile;     // input redirect: "ls < input.txt" → "input.txt"
    char*   outfile;    // output redirect: "ls > out.txt" → "out.txt"
    int     append;     // 1 = >>, 0 = >
    int     background; // 1 = &, 0 = foreground
} Command;
```

Lifecycle:
- Allocated in `parse_input()`
- Freed in `free_command(Command*)`

---

## 3. Pipeline

Represents a sequence of commands connected by pipes, and/or operators.

```c
// my_shell.h
typedef struct {
    Command** cmds;     // array of Command*, length = count
    int       count;    // number of commands in pipe chain
    int       connector;// 0=none, 1=&&, 2=||, 3=;
} Pipeline;
```

Examples:
```
"ls"              → Pipeline { cmds=[cmd0], count=1, connector=0 }
"ls | wc"         → Pipeline { cmds=[cmd0,cmd1], count=2, connector=0 }
"make && ./test"  → Pipeline { cmds=[cmd0], count=1, connector=1 }
                    Pipeline { cmds=[cmd1], count=1, connector=0 }
```

Lifecycle:
- Allocated in `parse_input()`
- Freed in `free_pipeline(Pipeline*)`

---

## 4. Job

Represents a background or suspended process.

```c
// jobs.h
#define MAX_JOBS 64
#define MAX_CMD_LEN 256

typedef enum {
    JOB_RUNNING,   // background, still running
    JOB_STOPPED,   // suspended via Ctrl+Z
    JOB_DONE       // exited, pending notification
} job_status_t;

typedef struct {
    int          job_id;            // 1-based display id: [1], [2], ...
    pid_t        pid;               // process id
    pid_t        pgid;              // process group id (for tcsetpgrp)
    char         command[MAX_CMD_LEN]; // original command string
    job_status_t status;
    int          exit_code;         // filled when DONE
} Job;

typedef struct {
    Job  jobs[MAX_JOBS];
    int  count;
    int  next_id;  // next job_id to assign
} JobTable;
```

---

## 5. History

```c
// history.h
#define HISTORY_MAX 1000

typedef struct {
    char** entries;     // heap-allocated array of strings
    int    size;        // current number of entries
    int    capacity;    // allocated capacity
    int    nav_index;   // current arrow-key navigation position
} History;
```

---

## 6. Shell State (shell_loop locals)

Not a struct — but these locals constitute the shell's runtime state:

```c
char**   env;              // current environment
int      env_is_heap;      // 0=original argv env, 1=malloc'd
char*    initial_dir;      // cwd at shell start (for cd with no args)
History  hist;             // command history
int      last_exit_status; // value of $?
JobTable jobs;             // background job table
```

---

## 7. Function Signatures Reference

### main.c
```c
int  main(int argc, char** argv, char** env);
void shell_loop(char** env);
int  dispatch(Pipeline* p, char*** env, int* last_status, JobTable* jobs, char* init_dir);
```

### executor.c
```c
int    executor(Command* cmd, char** env, JobTable* jobs, int* last_status);
int    execute_pipeline(Pipeline* p, char** env, JobTable* jobs);
void   child_process(Command* cmd, char** env);
int    apply_redirections(Command* cmd);
char*  get_path(char** env);
char** split_paths(char* paths, int* count);
void   free_paths(char** list, int count);
```

### builtins.c
```c
int    command_cd(char** args, char* initial_dir);
int    command_pwd(void);
int    command_echo(char** args, char** env);
int    command_env(char** env);
int    command_which(char** args, char** env);
char** command_setenv(char** args, char** env);
char** command_unsetenv(char** args, char** env);
int    command_jobs(char** args, JobTable* jobs);
int    command_fg(char** args, JobTable* jobs);
int    command_bg(char** args, JobTable* jobs);
```

### signals.c
```c
void setup_signals(void);
void sigchld_handler(int sig);
```

### jobs.c
```c
void  jobs_init(JobTable* jt);
int   job_add(JobTable* jt, pid_t pid, pid_t pgid, const char* cmd);
Job*  job_find(JobTable* jt, int job_id);
void  job_remove(JobTable* jt, int job_id);
void  job_print(Job* j);
void  jobs_check_done(JobTable* jt);
```

### input_parser.c
```c
Pipeline* parse_input(const char* input);
void      free_pipeline(Pipeline* p);
void      free_command(Command* cmd);
char*     expand_variables(const char* token, char** env, int last_status);
```

### helpers.c
```c
size_t my_strlen(const char* s);
int    my_strcmp(const char* a, const char* b);
int    my_strncmp(const char* a, const char* b, size_t n);
char*  my_strdup(const char* s);
char*  my_strchr(const char* s, int c);
char*  my_strcpy(char* dst, const char* src, size_t n);
char*  my_strtok(char* s, const char* delim);
char*  my_getenv(const char* name, char** env);
int    count_env_vars(char** env);
```

### history.c
```c
void  history_init(History* h);
void  history_add(History* h, const char* entry);
char* history_prev(History* h);
char* history_next(History* h);
void  history_free(History* h);
char* read_input(History* h);
```

---

## 8. Constants

```c
// my_shell.h
#define MAX_ARGS     1024
#define MAX_JOBS     64
#define MAX_CMD_LEN  256
#define HISTORY_MAX  1000
```
