#include "my_shell.h"


// 0 for same
//<0 for str1 <str2
//>0 for str1 >str2
int my_strcmp(const char* str1,const char* str2){

    while(*str1 && (*str1 == *str2)){
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 -*(unsigned char*)str2;
}

int my_strlen(const char* str){
    size_t len =0;
    while(*str){
        len++;
        str++;
    }
    return len;
}

int my_strncmp(const char* str1,const char* str2,size_t n){
    size_t i=0;

    while(i <n && str1[i] && str2[i]){
        if(str1[i]!=str2[i]){
            return (unsigned char)str1[i] -(unsigned char)str2[i];
        }
        i++;
    }
    if(i==n){
        return 0;
    }
    return (unsigned char)str1[i] -(unsigned char)str2[i];
}

char* my_getenv(const char* name,char** env){
    if(name ==NULL|| env == NULL){
        return NULL;
    }
    size_t name_len = my_strlen(name);

    for(int i=0;env[i];i++){
        if(my_strncmp(env[i],name,name_len) == 0 && env[i][name_len] == '='){
            return &env[i][name_len+1];
        }
    }
    return NULL;
}

// Helper for setEnv

int count_env_vars(char** env){
    int count =0;
    while(env[count]){
        count++;
    }
    return count;
}

char* my_strdup(const char* str) {
    if (str == NULL)
        return NULL;

    size_t len = 0;
    while (str[len])      // find length
        len++;

    char* copy = malloc((len + 1) * sizeof(char));
    if (!copy) {
        perror("malloc");
        return NULL;
    }

    for (size_t i = 0; i <= len; i++)  // copy including '\0'
        copy[i] = str[i];

    return copy;
}


char* my_strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c)
            return (char*)str;
        str++;
    }
    // Check for null terminator search
    if ((char)c == '\0')
        return (char*)str;

    return NULL;
}

// Tokenizes a string by splitting it based on a set of delimiter characters.
char* my_strtok(char* input_string, const char* delimiter)
{
    static char* next_token = NULL;

    // If input is NULL
    if (input_string == NULL) {
        input_string = next_token;
    }
    if (input_string == NULL) {
        return NULL;
    }

    while (*input_string && my_strchr(delimiter, *input_string)) {
        input_string++;
    }

    if (*input_string == '\0') {
        next_token = NULL;
        return NULL;
    }

    char* token = input_string;

    while (*input_string && !my_strchr(delimiter, *input_string)) {
        input_string++;
    }

    if (*input_string) {
        *input_string = '\0';
        next_token = input_string + 1;
    } else {
        next_token = NULL;
    }

    return token; 
}

// copy from source to destination
char* my_strcpy(char* dest,char* src ,size_t n){
    size_t i;

    for (i = 0; src[i] != '\0'; i++)
    {
        dest[i] = src[i];   
    }

    for(;i<n;i++){
        dest[i] = '\0';
    }
    return dest;
}