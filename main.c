#include <stdio.h>

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
        getline(&input,&input_size,stdin);
        
    }

}

int main(int argc, char** argv, char** env){
    
    (void)argc;
    (void)argv;

    shell_loop(env);

    return 0;
}

