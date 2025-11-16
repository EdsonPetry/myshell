#include "dynamic_array.h"
#include <stdio.h>
#include <stdlib.h>

// dynamic array implementation based on code from
// https://stackoverflow.com/a/3536261
// Posted by casablanca, modified by community.
// Retrieved 2025-11-15, License - CC BY-SA 4.0
// changed to handle strings instead of ints

void initArray(Array *a, size_t initialSize) {
  a->array = malloc(initialSize * sizeof(char *));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, char *element) {
  if (a->used == a->size) {
    a->size *= 2;
    a->array = realloc(a->array, a->size * sizeof(char *));
  }
  a->array[a->used++] = element;
}

void freeArray(Array *a) {
  // free each string
  for (size_t i = 0; i < a->used; i++) {
    free(a->array[i]);
  }
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}

void printArray(Array *a) {
  for (size_t i = 0; i < a->used; i++) {
    printf("%s\n", a->array[i]);
  }
}
