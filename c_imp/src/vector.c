#include "vector.h"

vector *new_vector() {
	int default_length = 8;

	vector *ret = malloc(sizeof(vector));

	if (ret == NULL) {
		return NULL;
	}

	ret->arr = calloc(default_length, sizeof(void *));

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

	clear_vector(v);
	free(v);
	return;
}

int append(vector *v, void *value) {
	if (v == NULL) {
		return -1;
	}
	if (v->num_elements == v->len) {
		v->len *= 2;
		void ** temp = (void **) realloc(v->arr, v->len * sizeof(void *));
		if (temp == NULL) {
			return -1;
		} else {
			v->arr = temp;
		}
	}
	v->arr[(v->num_elements)++] = value;
	return 1;
}

void *get(vector *v, int idx) {
	if (v == NULL) {
		return NULL;
	}
	if (idx >= v->num_elements || idx < 0) {
		return NULL;
	} else {
		return v->arr[idx];
	}
}

void *head(vector *v) {
	return get(v, 0);
}

void *tail(vector *v) {
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