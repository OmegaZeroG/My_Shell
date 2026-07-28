#include "my_shell.h"

void free_paths(char** list, int count) {
    for (int i = 0; i < count; i++)
        free(list[i]);
    free(list);
}

// Fetch the PATH from the environment
char* get_path(char** env) {
    for (int i = 0; env[i]; i++) {
        if (my_strncmp(env[i], "PATH=", 5) == 0)
            return my_strdup(env[i] + 5);
    }
    return NULL;
}

// Split PATH into its individual directories
char** split_paths(char* paths, int* count) {
    char** result = NULL;
    char* token;
    size_t size_of_path = my_strlen(paths);
    char* paths_copy = malloc(size_of_path + 1);
    if (!paths_copy) return NULL;
    my_strcpy(paths_copy, paths, size_of_path + 1);
    paths_copy[size_of_path] = '\0';

    token = my_strtok(paths_copy, ":");
    *count = 0;

    while (token) {
        char** grown = realloc(result, (*count + 1) * sizeof(char*));
        if (!grown) {
            perror("realloc");
            free_paths(result, *count);
            free(paths_copy);
            return NULL;
        }
        result = grown;
        result[*count] = my_strdup(token);
        if (!result[*count]) {
            perror("my_strdup");
            free_paths(result, *count);
            free(paths_copy);
            return NULL;
        }
        (*count)++;
        token = my_strtok(NULL, ":");
    }
    free(paths_copy);
    return result;
}

// Applies a Command's redirections to the CURRENT process's stdin/stdout.
// Only ever called inside a forked child, right before running the command.
int apply_redirections(Command* cmd) {
    if (cmd->infile) {
        int fd = open(cmd->infile, O_RDONLY);
        if (fd == -1) {
            perror(cmd->infile);
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (cmd->outfile) {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        int fd = open(cmd->outfile, flags, 0644);
        if (fd == -1) {
            perror(cmd->outfile);
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }

    return 0;
}

// A forked child that runs a built-in must not exit via the regular libc
// exit(): exit() flushes and *closes* every open stdio stream, including
// stdin. If stdin is a buffered FILE* over a regular file (e.g. the shell
// was invoked as `./my_shell < script.txt`) and there is still unread data
// sitting in that buffer, fclose() pushes those unread bytes back onto the
// underlying fd with lseek() so a future reader doesn't lose them. But that
// fd's file offset is *shared* with the parent shell (fork() duplicates the
// descriptor, not the underlying open file description) — so every forked
// builtin that exits this way rewinds the parent's read position a little
// further, and after enough of them the parent ends up back at the start of
// the script and re-executes it forever. `_exit()` skips all of that stdio
// cleanup entirely, so we flush our own output by hand first and use it
// everywhere a child terminates.
static void child_exit(int code) {
    fflush(stdout);
    fflush(stderr);
    _exit(code);
}

// Runs inside a forked child. Either dispatches to a built-in (so pipes and
// redirection work with echo/pwd/env/which the same way they do with any
// external program), or resolves the command through PATH and execve's it.
// Never returns.
void child_process(Command* cmd, char** env) {
    char** args = cmd->argv;
    const char* name = args[0];

    if (my_strcmp(name, "pwd") == 0)   child_exit(command_pwd());
    if (my_strcmp(name, "echo") == 0)  child_exit(command_echo(args, env));
    if (my_strcmp(name, "env") == 0)   child_exit(command_env(env));
    if (my_strcmp(name, "which") == 0) child_exit(command_which(args, env));

    // cd/setenv/unsetenv/exit only make sense in the parent shell; if they
    // show up here it's because they were used mid-pipeline (e.g.
    // "cd /tmp | echo hi"), which real shells also run in a throwaway
    // subshell whose side effects don't survive the pipe. We honour that:
    // run them for effect (mostly moot) and exit without crashing.
    if (my_strcmp(name, "cd") == 0) {
        char* cwd = getcwd(NULL, 0);
        int rc = command_cd(args, cwd);
        free(cwd);
        child_exit(rc);
    }
    if (my_strcmp(name, "setenv") == 0 || my_strcmp(name, "unsetenv") == 0 ||
        my_strcmp(name, "exit") == 0 || my_strcmp(name, "quit") == 0) {
        child_exit(0);
    }

    // A path with a '/' in it is run directly, no PATH search.
    if (my_strchr(name, '/')) {
        execve(name, args, env);
        perror(name);
        child_exit(127);
    }

    char* path_string = get_path(env);
    if (!path_string) {
        fprintf(stderr, "my_shell: PATH not set\n");
        child_exit(127);
    }

    int num_paths;
    char** path_list = split_paths(path_string, &num_paths);
    free(path_string);

    if (!path_list) {
        fprintf(stderr, "my_shell: failed to resolve PATH\n");
        child_exit(127);
    }

    for (int i = 0; i < num_paths; i++) {
        size_t len = my_strlen(path_list[i]) + my_strlen(name) + 2;
        char* full_path = malloc(len);
        if (!full_path) continue;

        snprintf(full_path, len, "%s/%s", path_list[i], name);
        execve(full_path, args, env); // only returns on failure
        free(full_path);
    }

    free_paths(path_list, num_paths);
    fprintf(stderr, "my_shell: %s: command not found\n", name);
    child_exit(127);
}

// Executes an N-command pipeline: forks one child per command, wires their
// stdin/stdout together with N-1 pipes, applies each command's own
// redirections, and (for foreground pipelines) waits for all children,
// returning the exit status of the last one — exactly like a real shell.
int execute_pipeline_chain(Pipeline* p, char** env) {
    int n = p->count;
    int(*pipefds)[2] = NULL;

    if (n > 1) {
        pipefds = malloc(sizeof(int[2]) * (n - 1));
        for (int i = 0; i < n - 1; i++) {
            if (pipe(pipefds[i]) == -1) {
                perror("pipe");
                for (int k = 0; k < i; k++) {
                    close(pipefds[k][0]);
                    close(pipefds[k][1]);
                }
                free(pipefds);
                return 1;
            }
        }
    }

    pid_t* pids = malloc(sizeof(pid_t) * n);

    // Belt-and-suspenders alongside the line-buffering set up in main():
    // guarantee no unflushed parent output survives into a child's copy of
    // the stdout buffer, no matter what buffering mode is active.
    fflush(stdout);

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            pids[i] = -1;
            continue;
        }

        if (pid == 0) {
            reset_child_signals();

            if (i > 0)     dup2(pipefds[i - 1][0], STDIN_FILENO);
            if (i < n - 1) dup2(pipefds[i][1], STDOUT_FILENO);
            for (int k = 0; k < n - 1; k++) {
                close(pipefds[k][0]);
                close(pipefds[k][1]);
            }

            if (apply_redirections(p->cmds[i]) == -1)
                child_exit(EXIT_FAILURE);

            if (p->background)
                setpgid(0, 0);

            child_process(p->cmds[i], env); // never returns
        }

        pids[i] = pid;
    }

    for (int k = 0; k < n - 1; k++) {
        close(pipefds[k][0]);
        close(pipefds[k][1]);
    }
    free(pipefds);

    if (p->background) {
        printf("[bg] launched pid %d (%s)\n", pids[n - 1], p->cmds[0]->argv[0]);
        for (int i = 0; i < n; i++) {
            if (pids[i] != -1)
                job_add(pids[i], p->cmds[0]->argv[0]);
        }
        free(pids);
        return 0;
    }

    int status = 0, last_status = 0;
    for (int i = 0; i < n; i++) {
        if (pids[i] == -1) continue;
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
            continue;
        }
        if (i == n - 1) {
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                printf("Process terminated by signal: %d\n", WTERMSIG(status));
                last_status = 128 + WTERMSIG(status);
            }
        }
    }

    free(pids);
    return last_status;
}

// Decides whether a single-command, non-backgrounded, non-redirected
// pipeline is one of the state-mutating built-ins that must run directly in
// the parent shell (cd/exit/setenv/unsetenv change state that only makes
// sense if it survives past this one command), or whether it should go
// through the general fork-based pipeline executor.
int run_pipeline(Pipeline* p, char*** env_ptr, int* env_is_heap, char* initial_directory) {
    if (p->count == 1 && !p->background) {
        Command* cmd = p->cmds[0];
        const char* name = cmd->argv[0];
        int has_redirection = (cmd->infile != NULL || cmd->outfile != NULL);

        if (!has_redirection) {
            if (my_strcmp(name, "cd") == 0) {
                return command_cd(cmd->argv, initial_directory);
            }
            if (my_strcmp(name, "exit") == 0 || my_strcmp(name, "quit") == 0) {
                exit(EXIT_SUCCESS);
            }
            if (my_strcmp(name, "setenv") == 0) {
                char** new_env = command_setenv(cmd->argv, *env_ptr);
                if (new_env != *env_ptr) {
                    if (*env_is_heap) {
                        for (int i = 0; (*env_ptr)[i]; i++) free((*env_ptr)[i]);
                        free(*env_ptr);
                    }
                    *env_ptr = new_env;
                    *env_is_heap = 1;
                }
                return 0;
            }
            if (my_strcmp(name, "unsetenv") == 0) {
                char** new_env = command_unsetenv(cmd->argv, *env_ptr);
                if (new_env != *env_ptr) {
                    if (*env_is_heap) {
                        for (int i = 0; (*env_ptr)[i]; i++) free((*env_ptr)[i]);
                        free(*env_ptr);
                    }
                    *env_ptr = new_env;
                    *env_is_heap = 1;
                }
                return 0;
            }
        }
    }

    return execute_pipeline_chain(p, *env_ptr);
}
