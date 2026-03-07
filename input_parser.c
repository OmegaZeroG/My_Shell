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
        
        token = &input[i];

        while(input[i] && input[i] !=' '){
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