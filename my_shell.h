#ifndef MY_SHELL_H
#define MY_SHELL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stddef.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT     1024
#define MAX_PATH_LEN  256
#define HISTORY_SIZE  100
#define MAX_ARGS      256
#define MAX_JOBS      64
#define MAX_CMD_LEN   256

typedef struct
{
    char *commands[HISTORY_SIZE];
    int count;
    int current;
} History;

// A single command: argv plus its own redirections.
typedef struct {
    char** argv;        // NULL-terminated argument list
    int    argc;        // number of arguments (excluding NULL terminator)
    char*  infile;       // "<" target, NULL if none
    char*  outfile;      // ">" or ">>" target, NULL if none
    int    append;       // 1 = ">>", 0 = ">"
} Command;

// A pipeline: cmd1 | cmd2 | ... | cmdN, plus how it connects to the *next*
// pipeline on the same input line.
typedef struct {
    Command** cmds;
    int       count;       // number of commands in the pipe chain
    int       connector;   // 0 = none/';', 1 = '&&', 2 = '||'
    int       background;  // 1 if the pipeline ends in '&'
} Pipeline;

// Parsing
Pipeline** parse_input(const char* input, int* out_count);
void free_command(Command* cmd);
void free_pipeline(Pipeline* p);
void free_pipelines(Pipeline** pipelines, int count);
void expand_pipeline(Pipeline* p, char** env, int last_status);

// Built-in command implementations
int command_cd(char** args, char* initial_directory);
int command_pwd(void);
int command_echo(char** args, char** env);
int command_env(char** env);
int command_which(char** args, char** env);

char** command_setenv(char** args, char** env);
char** command_unsetenv(char** args, char** env);

// Dispatch / execution
int  run_pipeline(Pipeline* p, char*** env_ptr, int* env_is_heap, char* initial_directory);
int  execute_pipeline_chain(Pipeline* p, char** env);
void child_process(Command* cmd, char** env);
int  apply_redirections(Command* cmd);
char* get_path(char** env);
char** split_paths(char* paths, int* count);
void free_paths(char** list, int count);

// Helpers (no <string.h> used anywhere in this project)
int my_strncmp(const char* str1, const char* str2, size_t n);
int my_strcmp(const char* str1, const char* str2);
size_t my_strlen(const char* str);
char* my_getenv(const char* name, char** env);
int count_env_vars(char** env);
char* my_strdup(const char* str);
char* my_strchr(const char* str, int c);
char* my_strcpy(char* dest, const char* src, size_t n);
char* my_strtok(char* input_string, const char* delimiter);

// History
void history_init(History* hist);
void history_add(History* hist, char* cmd);
void history_free(History* hist);
void enable_raw_mode(struct termios* orig);
void disable_raw_mode(struct termios* orig);
char* read_input(History* hist);

// Signals
void setup_parent_signals(void);
void reset_child_signals(void);
extern volatile sig_atomic_t g_sigchld_received;

// Background job tracking (reaped via SIGCHLD, no fg/bg job control)
void job_add(pid_t pid, const char* cmd);
void reap_background_jobs(void);

#endif
