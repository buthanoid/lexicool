enum { FALSE = 0, TRUE = 1 }; // used only for assignment

int max_int (int a, int b);

// adapt capacity of a stack
int adapt_capacity (int nb_elts, int * ref_capacity, size_t size_elt, void ** ref_elts);

typedef struct String_Builder String_Builder;

struct String_Builder {
	int nb_chars; // includes '\0'
	int capacity;
	char * chars;
};

String_Builder new_string_builder (int capacity);
void append (String_Builder * builder, char * other_string);
