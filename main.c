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
    
    (void)env;
    (void)initial_directory;
    

    if(my_strcmp(args[0],"cd")==0 ){
        
        return command_cd(args,initial_directory);
    }
    else if(my_strcmp(args[0],"pwd")==0){
        
        return command_pwd();
    }
    else if(my_strcmp(args[0],"echo")== 0){
         command_echo(args,env);
    }
    else if(my_strcmp(args[0],"env")== 0){
        command_env(env);
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
    
}



void shell_loop(char** env){
    char* input  = NULL;
    size_t input_size =0;

    char** args;
    char* initial_directory = getcwd(NULL,0);

    while(1){
        printf("[my_shell]$ ");
        fflush(stdout);

        ssize_t byte_read = getline(&input,&input_size,stdin);

        if( byte_read == -1){
            printf("\n");
            break;
        } //End of the file (EOF), ctrl + Z (windows) and ctrl + D (linus/mac)

        // printf("Input: %s",input);

        args = parse_input(input);

        // for(int i=0;args[i];i++){
        //     printf("ARGS: %s\n",args[i]);
        // }
        if(!args[0]){
            return;
        }else if(my_strcmp(args[0],"setenv") == 0){
            env = command_setenv(args,env);
        }
        else if(my_strcmp(args[0],"unsetenv") == 0){
            env = command_unsetenv(args,env);
        }
        else{
            shell_buildins(args,env,initial_directory);
        }

        
    }
    free_tokens(args);
    free(input);
}

int main(int argc, char** argv, char** env){
    
    (void)argc;
    (void)argv;
    (void)env;

    shell_loop(env);

    return 0;
}

