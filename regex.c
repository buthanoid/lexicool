#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nda.h"

enum { FALSE, TRUE };

typedef enum Tag_Regex Tag_Regex;
typedef struct Regex Regex;

enum Tag_Regex { REGEX_CHARACTER, REGEX_EPSILON, REGEX_SEQUENCE, REGEX_BRANCH, REGEX_REPEAT };

// this struct is used as a tagged union, with the tag being tag_regex
// I prefered a small memory overhead rather than the union complexity
struct Regex {
    Tag_Regex tag_regex; // used by all REGEX

    int character; // used by REGEX_CHARACTER
    Regex * one; // used by REGEX_SEQUENCE, REGEX_BRANCH, REGEX_REPEAT
    Regex * two; // used by REGEX_SEQUENCE, REGEX_BRANCH
    int min; // used by REGEX_REPEAT
    int max; // used by REGEX_REPEAT
};

NDA regex_to_nda (Regex regex);
int aux_length_regex_to_nda (Regex regex);
Node * aux_regex_to_nda (Regex regex, NDA * nda, Node * start_node);

char * regex_to_string (Regex regex);
int aux_length_regex_to_string (Regex regex);
int aux_regex_to_string (Regex regex, char * result_string);

int main (char ** argv) {

    Regex regex0;
    regex0.tag_regex = REGEX_CHARACTER;
    regex0.character = 12;

    Regex regex1;
    regex1.tag_regex = REGEX_CHARACTER;
    regex1.character = 4000;

    Regex regex2;
    regex2.tag_regex = REGEX_SEQUENCE;
    regex2.one = &regex0;
    regex2.two = &regex1;

    Regex regex3;
    regex3.tag_regex = REGEX_REPEAT;
    regex3.min = 1;
    regex3.max = 3;
    regex3.one = &regex2;

    printf("%i\n", aux_length_regex_to_string(regex3));
    char * string_regex = regex_to_string(regex3);
    printf("%s\n", string_regex);
    free(string_regex);

	NDA nda = regex_to_nda(regex3);
	printf("%i %i\n", nda.nb_nodes, nda.nodes[0].nb_arrows);
}


NDA regex_to_nda (Regex regex) {

	int nb_nodes = aux_length_regex_to_nda(regex);

	NDA nda = create_NDA(nb_nodes, 0); // TODO compute nb of counters

	Node * init_node = add_Node(&nda, FALSE, 2);

	aux_regex_to_nda(regex, &nda, init_node);

	return nda;
}

int aux_length_regex_to_nda (Regex regex) {
    int result_length, result_one, result_two;
	switch (regex.tag_regex) {
		case REGEX_EPSILON:
		case REGEX_CHARACTER:
			// two nodes with an arrow
			// (0)--a-->(1)
			result_length = 2;
			break;
		case REGEX_SEQUENCE:
		case REGEX_BRANCH:
			result_one = aux_length_regex_to_nda(*regex.one);
			result_two = aux_length_regex_to_nda(*regex.two);
			result_length = result_one + result_two;
		    if (regex.tag_regex == REGEX_SEQUENCE) result_length --;
			break;
        case REGEX_REPEAT:
            result_one = aux_length_regex_to_nda(*regex.one);
            result_length = result_one + 4;
            break;
	}
	return result_length;
}

Node * aux_regex_to_nda (Regex regex, NDA * nda, Node * start_node) {
	Node * end_node, * node_one, * node_two, * node_three;
    Arrow * arrow;
	switch (regex.tag_regex) {
		case REGEX_EPSILON:
		case REGEX_CHARACTER:
			// wet set arrow capacity to 2 because we will never need more than 2 arrows
			// we choose to allocate largest possible, rather than to allocate often
			end_node = add_Node(nda, TRUE, 2);
			add_arrow(start_node, regex.character, end_node->num, 0);
			break;
		case REGEX_SEQUENCE:
			node_one = aux_regex_to_nda(*regex.one, nda, start_node);
            end_node = aux_regex_to_nda(*regex.two, nda, node_one);
			node_one->success = FALSE;
			break;
		case REGEX_BRANCH:
			node_one = aux_regex_to_nda(*regex.one, nda, start_node);
			node_two = aux_regex_to_nda(*regex.two, nda, start_node);
            end_node = add_Node(nda, TRUE, 2);
			node_one->success = FALSE;
			node_two->success = FALSE;
			add_arrow(node_one, REGEX_EPSILON, end_node->num, 0);
			add_arrow(node_two, REGEX_EPSILON, end_node->num, 0);
            break;
        case REGEX_REPEAT:
            node_one = add_Node(nda, FALSE, 2);
            node_two = add_Node(nda, FALSE, 1);
            node_three = aux_regex_to_nda(*regex.one, nda, node_two);
            end_node = add_Node(nda, TRUE, 2);

            node_three->success = FALSE;

            arrow = add_arrow(start_node, REGEX_EPSILON, node_one->num, 1);
            arrow->counters_actions[0] = new_counter_action(0, ACTION_SET, 0);

            arrow = add_arrow(node_one, REGEX_EPSILON, node_two->num, 1);
            // regex.max - 1, because that AT_MOST is inclusive
            // we must not follow the arrow if we already reached regex.max
            arrow->counters_actions[0] = new_counter_action(0, ACTION_AT_MOST, regex.max - 1);

            arrow = add_arrow(node_three, REGEX_EPSILON, node_one->num, 1);
            arrow->counters_actions[0] = new_counter_action(0, ACTION_ADD, 1);

            arrow = add_arrow(node_one, REGEX_EPSILON, end_node->num, 2);
            arrow->counters_actions[0] = new_counter_action(0, ACTION_AT_LEAST, regex.min);
            arrow->counters_actions[1] = new_counter_action(0, ACTION_AT_MOST, regex.max);
            break;
	}
	return end_node;
}

char * regex_to_string (Regex regex) {
    int string_length = aux_length_regex_to_string(regex);
    char * string_regex = malloc(1 + string_length * sizeof(char));
    aux_regex_to_string(regex, string_regex);
    strcpy(string_regex + string_length, "\0");
    return string_regex;
}


// compute the length necessary to store the string
int aux_length_regex_to_string (Regex regex) {
    char buffer[100];
    int result_length, result_one, result_two;
    switch (regex.tag_regex) {
        case REGEX_EPSILON:
            // number of characters in string "EPS"
            result_length = 3;
            break;
        case REGEX_CHARACTER:
            result_length = sprintf(buffer, "%i", regex.character);
            break;
        case REGEX_SEQUENCE:
            // sum of the lengths of the two branches + 1 for symbol '.'
            result_one = aux_length_regex_to_string(*regex.one);
            result_two = aux_length_regex_to_string(*regex.two);
            result_length = result_one + result_two + 1;
            break;
        case REGEX_BRANCH:
            // sum of the lengths of the two branches + 3 for symbosl '|' and '(' and ')'
            result_one = aux_length_regex_to_string(*regex.one);
            result_two = aux_length_regex_to_string(*regex.two);
            result_length = result_one + result_two + 3;
            break;
        case REGEX_REPEAT:
            result_one = aux_length_regex_to_string(*regex.one);
            result_two = sprintf(buffer, "(){%i,%i}", regex.min, regex.max);
            result_length = result_one + result_two;
            break;
    }
    return result_length;
}

int aux_regex_to_string (Regex regex, char * result_string) {
    int nb_written = 0;
    switch (regex.tag_regex) {
        case REGEX_EPSILON:
            nb_written += sprintf(result_string + nb_written, "EPS");
            break;
        case REGEX_CHARACTER:
            nb_written += sprintf(result_string + nb_written, "%i", regex.character);
            break;
        case REGEX_SEQUENCE:
            nb_written += aux_regex_to_string(*regex.one, result_string + nb_written);
            nb_written += sprintf(result_string + nb_written, ".");
            nb_written += aux_regex_to_string(*regex.two, result_string + nb_written);
            break;
        case REGEX_BRANCH:
            nb_written += sprintf(result_string + nb_written, "(");
            nb_written += aux_regex_to_string(*regex.one, result_string + nb_written);
            nb_written += sprintf(result_string + nb_written, "|");
            nb_written += aux_regex_to_string(*regex.two, result_string + nb_written);
            nb_written += sprintf(result_string + nb_written, ")");
            break;
        case REGEX_REPEAT:
            nb_written += sprintf(result_string + nb_written, "(");
            nb_written += aux_regex_to_string(*regex.one, result_string + nb_written);
            nb_written += sprintf(result_string + nb_written, "){%i,%i}", regex.min, regex.max);
            break;
    }
    return nb_written;
}
