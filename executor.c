#include "my_shell.h"

// Executes the command by forking and running  it in a child process
int executor(char** args, char** env) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
       
        child_process(args, env);
        perror("execve"); 
        exit(EXIT_FAILURE);
    } else {
        // parent waits
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }
        if (WIFSIGNALED(status))
            printf("Process terminated by signal: %d\n", WTERMSIG(status));
    }
    return 0;
}

int child_process(char** args, char** env) {
    char* path_string = get_path(env);
    if (!path_string) {
        fprintf(stderr, "PATH not found\n");
        exit(EXIT_FAILURE);
    }

    int num_paths;
    char** path_list = split_paths(path_string, &num_paths);
    free(path_string);

    if (!path_list) {
        fprintf(stderr, "Failed to split PATH\n");
        exit(EXIT_FAILURE);
    }

    // Try each path directory
    for (int i = 0; i < num_paths; i++) {
        // Build full path: /usr/bin + / + ls = /usr/bin/ls
        size_t len = my_strlen(path_list[i]) + my_strlen(args[0]) + 2;
        char* full_path = malloc(len);
        if (!full_path) continue;

        sprintf(full_path, "%s/%s", path_list[i], args[0]);

        execve(full_path, args, env);  // if it works, never returns
        free(full_path);
    }

    // If we get here, command not found
    fprintf(stderr, "%s: command not found\n", args[0]);
    exit(EXIT_FAILURE);
}

//Fetch the PATH from the environment-variable
char* get_path(char** env){
    for(int i=0;env[i];i++){
        if(my_strncmp(env[i],"PATH=",5) == 0){
            return my_strdup(env[i]+5);
        }
    }
    return NULL;
}

// Split the path into individual parts
char** split_paths(char* paths,int* count){
     
    char** result = NULL;
    char* token;
    size_t size_of_path = my_strlen(paths);
    char paths_copy[size_of_path + 1]; 

    my_strcpy(paths_copy,paths ,sizeof(paths_copy));
    paths_copy[sizeof(paths_copy)- 1] ='\0';

    token = my_strtok(paths_copy,":");
    *count =0;   

    while(token){
        result = realloc(result, ((*count+1)*sizeof(char*)));
        if (!result) {
            perror("realloc");
            for (int j = 0; j < *count; j++){
                free(result[j]);
            }
            free(result);
            return NULL;
        }
        result[*count] =my_strdup(token);
        if(!result[*count]){
            perror("my_strdup");
            return NULL;

        }

        (*count)++;
        token = my_strtok(NULL,":");    
    }

    return result;
}