#ifndef MY_SHELL_H
#define MY_SHELL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>

#define MAX_INPUT 1024  
 

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

//Helpers
int my_strncmp(const char* str1,const char* str2,size_t n);
int my_strcmp(const char* str1,const char* str2);
int my_strlen(const char* str);
char* my_getenv(const char* name,char** env);

#endif