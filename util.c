#include <stdlib.h>
#include <string.h>

#include "util.h"

int max_int (int a, int b) { return (a > b) ? a : b; }

int adapt_capacity (int nb_elts, int * ref_capacity, size_t size_elt, void ** ref_elts) {

	if (*ref_capacity >= nb_elts) {
        return TRUE;
    }
    else {
    	int new_capacity = max_int(nb_elts, (*ref_capacity) * 2);

    	void * new_elts = reallocarray(*ref_elts, new_capacity, size_elt);

    	if (new_elts == NULL) {
    		return FALSE;
    	}
    	else {
    		*ref_capacity = new_capacity;
    		*ref_elts = new_elts;
    		return TRUE;
    	}
    }
}

// capacity must be >= 1
String_Builder new_string_builder (int capacity) {
	String_Builder builder;
	builder.chars = malloc(capacity * sizeof(char));
	builder.chars[0] = '\0';
	builder.nb_chars = 1;
	builder.capacity = capacity;
	return builder;
}

void append (String_Builder * builder, char * other_string) {
	int other_length = strlen(other_string);
	int both_length = builder->nb_chars + other_length;

	int success = adapt_capacity(
		both_length, &builder->capacity, sizeof(char), (void **) &builder->chars);

	if (success) {
		memcpy(builder->chars + builder->nb_chars - 1, other_string, other_length + 1);
		builder->nb_chars += other_length;
	}
}
