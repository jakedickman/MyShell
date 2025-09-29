#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arraylist.h"

void initialize_list(arraylist *list, int starting_capacity) {
    list->content = malloc(starting_capacity * sizeof(char *));
    if (list->content == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }
    list->size = 0;
    list->capacity = starting_capacity;
}

void append_list(arraylist *list, char *element) {
    if (list->size == list->capacity) {
        list->capacity = list->capacity*2;
        list->content = realloc(list->content, list->capacity * sizeof(char *));
        if (list->content == NULL) {
            perror("Memory allocation failed");
            exit(1);
        }
    }
    list->content[list->size++] = element;
}

void free_list(arraylist *list) {
    for (int i = 0; i < list->size; i++) {
        free(list->content[i]);
    }
    free(list->content);
}