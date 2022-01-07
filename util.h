typedef struct String_Builder String_Builder;

String_Builder new_string_builder (int capacity);
void append (String_Builder * builder, char * other_string);

struct String_Builder {
	char * str;
	int length; // includes '\0'
	int capacity;
};
