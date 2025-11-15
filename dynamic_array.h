#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h>

typedef struct {
  char **array;
  size_t used;
  size_t size;
} Array;

void initArray(Array *a, size_t initialSize);
void insertArray(Array *a, char *element);
void freeArray(Array *a);

#endif
