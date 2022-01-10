enum { FALSE = 0, TRUE = 1 }; // used only for assignment

int adapt_capacity (int * capacity, void ** ref_array, int nb_elements, size_t element_size);

typedef struct String_Builder String_Builder;

struct String_Builder {
	char * str;
	int length; // includes '\0'
	int capacity;
};

String_Builder new_string_builder (int capacity);
void append (String_Builder * builder, char * other_string);
