#include "vector.h"

vector *new_vector() {
	int default_length = 8;

	vector *ret = malloc(sizeof(vector));

	if (ret == NULL) {
		return NULL;
	}

	ret->arr = calloc(default_length, sizeof(float *));

	if (ret->arr == NULL) {
		free(ret);
		return NULL;
	}

	ret->len = default_length;
	ret->num_elements = 0;
	return ret;
}

void free_vector(vector *v) {
	if (v == NULL) {
		return;
	}

	for (int i = 0; i < v->len; i++) {
		free(v->arr[i]);
	}
	free(v);
	return;
}

int append(vector *v, float *value) {
	if (v == NULL) {
		return -1;
	}
	if (v->num_elements == v->len) {
		v->len *= 2;
		float ** temp = (float **) realloc(v->arr, v->len * sizeof(float *));
		if (temp == NULL) {
			return -1;
		} else {
			v->arr = temp;
		}
	}
	v->arr[(v->num_elements)++] = value;
	return 1;
}

float *get(vector *v, int idx) {
	if (v == NULL) {
		return NULL;
	}
	if (idx >= v->num_elements || idx < 0) {
		return NULL;
	} else {
		return v->arr[idx];
	}
}

float *head(vector *v) {
	return get(v, 0);
}

float *tail(vector *v) {
	if (v == NULL) {
		return NULL;
	}
	return get(v, v->num_elements - 1);
}

int size(vector *v) {
	if (v == NULL) {
		return 0;
	}
	return v -> num_elements;
}

void clear_vector(vector * v) {
	if (v == NULL) {
		return;
	}
	for (int i = 0; i < v->len; i++) {
		free(v->arr[i]);
		v->arr[i] = NULL;
	}
	v->num_elements = 0;
	return;
}