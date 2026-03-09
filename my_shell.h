#ifndef MY_SHELL_H
#define MY_SHELL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stddef.h>
#include <sys/wait.h>

#define MAX_INPUT 1024  
#define MAX_PATH_LEN 256  

 

char** parse_input(char* input);
void free_tokens(char** tokens);

int shell_buildins(char** args, char** env, char* initial_directory);

//Built-in function implementations

int command_cd(char** args,char* inital_directory);
int command_pwd();
int command_echo(char** args,char** env);
int command_env(char** env);
int command_which(char** args,char** env);

char** command_setenv(char** args,char** env);
char** command_unsetenv(char** args,char** env);

// Executor Functions
int executor(char**args , char** env);
char** split_paths(char* paths,int* count);
char* get_path(char** env);
int child_process(char** args, char** env);


//Helpers
int my_strncmp(const char* str1,const char* str2,size_t n);
int my_strcmp(const char* str1,const char* str2);
int my_strlen(const char* str);
char* my_getenv(const char* name,char** env);
int count_env_vars(char** env);
char* my_strdup(const char* str);
char* my_strchr(const char* str, int c);
char* my_strcpy(char* dest,char* src ,size_t n);
char* my_strtok(char* input_string, const char* delimiter);
#endif