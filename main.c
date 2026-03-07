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

void shell_loop(char** env){
    char* input  = NULL;
    size_t input_size =0;

    char** args;

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

        for(int i=0;args[i];i++){
            printf("ARGS: %s\n",args[i]);
        }
    }
    free(input);
}

int main(int argc, char** argv, char** env){
    
    (void)argc;
    (void)argv;

    shell_loop(env);

    return 0;
}

