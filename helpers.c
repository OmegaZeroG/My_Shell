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