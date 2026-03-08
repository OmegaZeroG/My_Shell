#include "my_shell.h"

int command_cd(char** args,char* inital_dir){
    if(args[1] == NULL){
        printf("cd: expected argument\"cd[direcotry]\"\n");
    }
    else if(chdir(args[1]) == 0){
        printf("CD working!\n");
    }    
    else{perror("CD");}
    
    return 0;
}


int command_pwd(){
    char* cwd =NULL;
    size_t size =0;

    //dynamic allocation
    cwd = getcwd(NULL,0);

    if(cwd != NULL){
        printf("%s\n",cwd);
        free(cwd);
    }
    else{
        perror("getcwd");
    }
    return 0;
}

//
int command_echo(char** args, char** env) {
    // (void)env;
    int new_line = 1;
    int i = 1;

    if (args[1] != NULL && my_strcmp(args[1], "-n") == 0) {
        new_line = 0;
        i++;
    }

    for (; args[i]; i++) {
        if (args[i][0] == '$') {
            char* value =  my_getenv(args[i]+1,env);
            if(value){
                printf("%s",value);
            }else{
                printf(" ");
            }
        } else {
            printf("%s", args[i]);
        }

        if (args[i + 1] != NULL)   // space between words, not after last
            printf(" ");
    }

    if (new_line)       // newline ONCE after loop
        printf("\n");

    return 0;
}


int command_env(char** env){
    size_t index =0;
    while(env[index]){
        printf("%s\n",env[index]);
        index++;
    }
    // printf("\n");   
    return 0;
}


int command_which(char** args,char** env);


char** command_setenv(char** args,char** env);


char** command_unsetenv(char** args,char** env);

