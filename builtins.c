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


// Function to set an env variable
char** command_setenv(char** args, char** env) {

    if (args[1] == NULL) {
        printf("Usage: setenv VAR=value\nor\tsetenv <variable> <value>\n");
        return env;
    }

    int env_count = count_env_vars(env);
    char** new_env = malloc((env_count + 2) * sizeof(char*));
    if (!new_env) {
        perror("malloc");
        return env;
    }

    // Copy existing env
    for (int i = 0; i < env_count; i++) {
        new_env[i] = my_strdup(env[i]);
        if (!new_env[i]) {
            perror("strdup");
            for (int j = 0; j < i; j++)
                free(new_env[j]);
            free(new_env);
            return env;
        }
    }

    // Build new variable string
    char* new_var = NULL;

    if (args[2] != NULL) {
        // Format: setenv VAR value  →  "VAR=value"
        size_t len = my_strlen(args[1]) + my_strlen(args[2]) + 2; // +2 for '=' and '\0'
        new_var = malloc(len);
        if (new_var)
            sprintf(new_var, "%s=%s", args[1], args[2]);
    } else if (my_strchr(args[1], '=') != NULL) {
        // Format: setenv VAR=value  →  already correct format
        new_var = my_strdup(args[1]);
    } else {
        printf("Usage: setenv VAR=value\nor\tsetenv <variable> <value>\n");
        for (int i = 0; i < env_count; i++)
            free(new_env[i]);
        free(new_env);
        return env;
    }

    if (!new_var) {
        perror("malloc");
        for (int i = 0; i < env_count; i++)
            free(new_env[i]);
        free(new_env);
        return env;
    }

    // Check if variable already exists and update it
    size_t var_name_len = my_strchr(new_var, '=') - new_var;
    for (int i = 0; i < env_count; i++) {
        if (my_strncmp(new_env[i], new_var, var_name_len) == 0 
            && new_env[i][var_name_len] == '=') {
            free(new_env[i]);
            new_env[i] = new_var;   // replace existing var
            new_env[env_count] = NULL;
            return new_env;
        }
    }

    // Variable doesn't exist — append it
    new_env[env_count] = new_var;
    new_env[env_count + 1] = NULL;

    printf("new_env: %s\n",new_var);

    return new_env;
}

char** command_unsetenv(char** args, char** env) {
    if (!args[1]) {
        printf("Usage: unsetenv <variable>\n");
        return env;
    }

    int env_count = count_env_vars(env);
    char** new_env = malloc(env_count * sizeof(char*));
    if (!new_env) {
        perror("malloc");
        return env;
    }

    int j = 0, found = 0;
    size_t var_len = my_strlen(args[1]);

    for (int i = 0; i < env_count; i++) {
        if (my_strncmp(env[i], args[1], var_len) == 0  // ✅ args[1] not args[i]
            && env[i][var_len] == '=') {
            found = 1;
            // don't copy this one — it's removed
        } else {
            new_env[j++] = env[i];
        }
    }

    new_env[j] = NULL;  // ✅ NULL terminate

    if (!found) {
        printf("Variable %s not found\n", args[1]);
        free(new_env);
        return env;
    }

    return new_env;  // ✅ must return
}
