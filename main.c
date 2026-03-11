#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "my_shell.h"


//shell loop
//input parsing
//commands execution ex- cd , pwd, echo,env,setenv,unsetenv,which,,exit
//Execute external commands
//Manage environment variable
//manage Path
//Error Handling

//BuiltINS-echo, cd , pwd, echo,env,setenv,unsetenv,which,exit
//Binary- ls,cat
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
        // command_which(args,env);
    }
    else if(my_strcmp(args[0],"exit") == 0 || my_strcmp(args[0],"quit")== 0){
        //printf("BYII");
        exit(EXIT_SUCCESS);
    }
    else{
        executor(args , env);
        // NOT A BUILT IN COMMAND
    }
    return 0;
}



void shell_loop(char** env){
    char* input  = NULL;
    size_t input_size =0;

    char** args;
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


        // for(int i=0;args[i];i++){
        //     printf("ARGS: %s\n",args[i]);
        // }
        if(!args[0]){
            free_tokens(args);
            continue;
        }else if(my_strcmp(args[0],"setenv") == 0){
            env = command_setenv(args,env);
        }
        else if(my_strcmp(args[0],"unsetenv") == 0){
            env = command_unsetenv(args,env);
        }
        else{
            shell_buildins(args,env,initial_directory);
        }

        
        free_tokens(args);
    }
    history_free(&hist);
    free(initial_directory);

}

int main(int argc, char** argv, char** env){
    
    (void)argc;
    (void)argv;


    shell_loop(env);

    return 0;
}

