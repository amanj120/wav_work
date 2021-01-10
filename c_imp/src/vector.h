#ifndef VECTOR_H_GUARD
#define VECTOR_H_GUARD

#include <stdlib.h>

typedef struct vector{
	void **arr;
	int len;
	int num_elements;
} vector;

vector *new_vector();
int append(vector *v, void *value);
void free_vector(vector *v);
void *get(vector *v, int idx);
void *head(vector *v);
void *tail(vector *v);
int size(vector *v);
void clear_vector(vector *v);
#endif
