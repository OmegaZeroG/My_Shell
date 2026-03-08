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