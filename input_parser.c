#include "my_shell.h"



char** parse_input (char* input){
    
    size_t buffer_size = MAX_INPUT;
    char** tokens = malloc(buffer_size * sizeof(char*));
    char* token = NULL;

    size_t token_length=0;
    size_t position = 0;

    if(!tokens){
        perror("MALLOC");
        exit(EXIT_FAILURE);
    }

    for(size_t i=0; input[i];i++){
        //skiping white space caharacters
        while(input[i]==' '|| input[i]=='\n' ||input[i]=='\t'|| input[i]=='\a'){
            i++;
        }
        if(input[i] == '\0') break;

        token = &input[i];

        while(input[i] && input[i] !=' ' &&  input[i]!='\n' &&input[i]!='\t'&& input[i]!='\a'){
            token_length++;
            i++;
        }

        tokens[position] = malloc((token_length+1)*sizeof(char));

        if(!tokens[position]){
        perror("MALLOC");
        exit(EXIT_FAILURE);
        }

        for(size_t j=0;j < token_length;j++){
            tokens[position][j] = token[j];
        }

        tokens[position][token_length] = '\0' ; // NULL terminate Token
        position++;
        token_length = 0 ; //RESET this to next token
    }

    tokens[position] =NULL; //terminate the array with NULL 

    return tokens;
}

// Free Allocated Tokens

void free_tokens(char** tokens){
    if(tokens){
        return;
    }
    for(int i=0;tokens[i];i++){
        free(tokens[i]); // free each token 
    }

    free(tokens); // free array
}