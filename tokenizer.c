#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"

#define BUFFER_SIZE 1024

enum tok_t next_token(char **token, int reset) {
    static char *input = NULL; 
    static int position = 0;
    static char temp_buffer[BUFFER_SIZE];

    if (reset) { //Resets if resest = 1
        input = NULL;
        position = 0;
        return NL;
    }

    if (input == NULL && *token != NULL) {
        input = *token;
        position = 0;
    }

    while (input[position] != '\0' && isspace(input[position])) { //Skips spaces
        position++;
    }

    if (input[position] == '\0') { //Checks end of string
        return EOS;
    }

    //Checks for special characters
    char current_char = input[position];
    if (current_char == '<') {
        position++;
        return LT;
    } 
    if (current_char == '>') {
        position++;
        return GT;
    } 
    if (current_char == '|') {
        position++;
        return BAR;
    } 
    if (current_char == '\n') {
        position++;
        return NL;
    }

    int i = 0;
    while (input[position] != '\0' && !isspace(input[position]) &&
           input[position] != '<' && input[position] != '>' && input[position] != '|') {
        if (i < BUFFER_SIZE - 1) {
            temp_buffer[i++] = input[position++];
        } else {
            fprintf(stderr, "Error: Token exceeds buffer size\n");
            exit(1);
        }
    }
    temp_buffer[i] = '\0'; // Null-terminate the token

    size_t token_len = strlen(temp_buffer);
    *token = malloc(token_len + 1);
    if (*token == NULL) {
        perror("Memory allocation error");
        exit(1);
    }
    strcpy(*token, temp_buffer);
    return WORD;
}
