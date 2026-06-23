#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "my_shell.h"



int shell_buildins(char** args, char** env, char* initial_directory){
    

    

    if(my_strcmp(args[0],"cd")==0 ){
        
        return command_cd(args,initial_directory);
    }
    else if(my_strcmp(args[0],"pwd")==0){
        
        return command_pwd();
    }
    else if(my_strcmp(args[0],"echo")== 0){
        return command_echo(args,env);
    }
    else if(my_strcmp(args[0],"env")== 0){
        return command_env(env);
    }
    else if(my_strcmp(args[0],"which")== 0){
        return command_which(args,env);
    }
    else if(my_strcmp(args[0],"exit") == 0 || my_strcmp(args[0],"quit")== 0){
        
        exit(EXIT_SUCCESS);
    }
    else{
        executor(args , env);
        // NOT A BUILT IN COMMAND
    }
    return 0;
}



void shell_loop(char** env){

    char** args;
    int env_is_heap = 0;
    char* initial_directory = getcwd(NULL,0);

    History hist;
    history_init(&hist);

    while(1){
        printf("[my_shell]$ ");
        fflush(stdout);

        // read_input handles arrow keys, backspace, Ctrl+D
        char* input = read_input(&hist);

        if (!input) {
            // EOF (Ctrl+D)
            printf("\n");
            break;
        }

        if (input[0] == '\0') {
            free(input);
            continue;
        }

        // Save to history
        history_add(&hist, input);

        args = parse_input(input);
        free(input);


        
        if(!args[0]){
            free_tokens(args);
            continue;
        } else if(my_strcmp(args[0],"setenv") == 0){
            char** new_env = command_setenv(args, env);
            if (new_env != env) {
                if (env_is_heap) {
                    for (int i = 0; env[i]; i++)
                        free(env[i]);
                    free(env);
                }
                env = new_env;
                env_is_heap = 1;
            }
        }
        else if(my_strcmp(args[0],"unsetenv") == 0){
            char** new_env = command_unsetenv(args, env);
            if (new_env != env) {
                if (env_is_heap) {
                    for (int i = 0; env[i]; i++)
                        free(env[i]);
                    free(env);
                }
                env = new_env;
                env_is_heap = 1;
            }
        }
        else{
            shell_buildins(args,env,initial_directory);
        }

        
        free_tokens(args);
    }
    history_free(&hist);
    free(initial_directory);
    if (env_is_heap) {
        for (int i = 0; env[i]; i++)
            free(env[i]);
        free(env);
    }
}

int main(int argc, char** argv, char** env){
    
    (void)argc;
    (void)argv;


    shell_loop(env);

    return 0;
}

