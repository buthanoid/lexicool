#include <stdlib.h>
#include <string.h>

#include "util.h"

int max_int (int a, int b) { return (a > b) ? a : b; }

int adapt_capacity (int * capacity, void ** ref_array, int nb_elements, size_t element_size) {

	int new_capacity = max_int(nb_elements, (*capacity) * 2);

	void * new_array = reallocarray(*ref_array, new_capacity, element_size);

	if (new_array == NULL) {
		return FALSE;
	}
	else {
		*capacity = new_capacity;
		*ref_array = new_array;
		return TRUE;
	}
}

// capacity must be >= 1
String_Builder new_string_builder (int capacity) {
	String_Builder builder;
	builder.str = malloc(capacity * sizeof(char));
	builder.str[0] = '\0';
	builder.length = 1;
	builder.capacity = capacity;
	return builder;
}

void append (String_Builder * builder, char * other_string) {
	int other_length = strlen(other_string);
	int both_length = builder->length + other_length;

	int success = adapt_capacity(
		&builder->capacity, (void **) &builder->str, both_length, sizeof(char));

	if (success) {
		memcpy(builder->str + builder->length - 1, other_string, other_length + 1);
		builder->length += other_length;
	}
}
