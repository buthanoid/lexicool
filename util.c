#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "util.h"

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

	if (both_length > builder->capacity) {
		builder->capacity *= 2;
		if (both_length > builder->capacity) builder->capacity = both_length ;
		char * new_str = realloc(builder->str, builder->capacity * sizeof(char));
		if (new_str == NULL) {

			return;
		}
		else builder->str = new_str;
	}

	memcpy(builder->str + builder->length - 1, other_string, other_length + 1);
	builder->length += other_length;
}
