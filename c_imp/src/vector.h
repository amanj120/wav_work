#ifndef VECTOR_H_GUARD
#define VECTOR_H_GUARD

#include <stdlib.h>

typedef struct vector{
	float **arr;
	int len;
	int num_elements;
} vector;

vector *new_vector();
int append(vector *v, float *value);
void free_vector(vector *v);
float *get(vector *v, int idx);
float *head(vector *v);
float *tail(vector *v);
int size(vector *v);
void clear_vector(vector *v);
#endif
