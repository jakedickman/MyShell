#ifndef ARRAYLIST_H
#define ARRAYLIST_H

typedef struct {
    char **content; 
    int size;     
    int capacity; 
} arraylist;

void initialize_list(arraylist *list, int starting_capacity);
void append_list(arraylist *list, char *element);
void free_list(arraylist *list);

#endif