#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "automata.h"

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

Automata regex_to_automata (Regex regex);
int aux_length_regex_to_automata (Regex regex);
Node * aux_regex_to_automata (Regex regex, Automata * automata, Node * start_node);

char * regex_to_string (Regex regex);

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

    char * string_regex = regex_to_string(regex3);
    printf("%s\n\n", string_regex);
    free(string_regex);

    Automata automata = regex_to_automata(regex3);

    char * string_automata = automata_to_string(automata);
    printf("%s\n", string_automata);
    free(string_automata);

    int labels[] = { 12, 4000, 12, 4000, 12, 4000, 12, 4000 };
    Pipe success_walks = all_success_walks(automata, 0, 8, labels);

    int longest_nb_labels_used = 0;
	int longest_walk_node_num = 0;

	while (success_walks.length > 0) {
		Walk walk = pop(&success_walks);
		if (walk.nb_labels_used > longest_nb_labels_used) {
			longest_nb_labels_used = walk.nb_labels_used;
			longest_walk_node_num = walk.node->num;
		}
		free_walk(walk);
	}

    printf("%i labels used, landed in node %i\n", longest_nb_labels_used, longest_walk_node_num);

    free_automata(automata);
}


Automata regex_to_automata (Regex regex) {

    int nb_nodes = aux_length_regex_to_automata(regex);

    Automata automata = new_automata(nb_nodes, 1); // TODO compute nb of counters

    Node * init_node = add_node(&automata, FALSE, 2);

    aux_regex_to_automata(regex, &automata, init_node);

    return automata;
}

int aux_length_regex_to_automata (Regex regex) {
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
            result_one = aux_length_regex_to_automata(*regex.one);
            result_two = aux_length_regex_to_automata(*regex.two);
            result_length = result_one + result_two;
            if (regex.tag_regex == REGEX_SEQUENCE) result_length --;
        break;
        case REGEX_REPEAT:
            result_one = aux_length_regex_to_automata(*regex.one);
            result_length = result_one + 4;
        break;
    }
    return result_length;
}

Node * aux_regex_to_automata (Regex regex, Automata * automata, Node * start_node) {
    Node * end_node, * node_one, * node_two, * node_three;
    Arrow * arrow;
    switch (regex.tag_regex) {
        case REGEX_EPSILON:
            end_node = add_node(automata, TRUE, 2);
            add_arrow(start_node, LABEL_EPSILON, end_node->num, 0);
        break;
        case REGEX_CHARACTER:
            // wet set arrow capacity to 2 because we will never need more than 2 arrows
            // we choose to allocate largest possible, rather than to allocate often
            end_node = add_node(automata, TRUE, 2);
            add_arrow(start_node, regex.character, end_node->num, 0);
        break;
        case REGEX_SEQUENCE:
            node_one = aux_regex_to_automata(*regex.one, automata, start_node);
            end_node = aux_regex_to_automata(*regex.two, automata, node_one);
            node_one->success = FALSE;
        break;
        case REGEX_BRANCH:
            node_one = aux_regex_to_automata(*regex.one, automata, start_node);
            node_two = aux_regex_to_automata(*regex.two, automata, start_node);
            end_node = add_node(automata, TRUE, 2);
            node_one->success = FALSE;
            node_two->success = FALSE;
            add_arrow(node_one, LABEL_EPSILON, end_node->num, 0);
            add_arrow(node_two, LABEL_EPSILON, end_node->num, 0);
        break;
        case REGEX_REPEAT:
            node_one = add_node(automata, FALSE, 2);
            node_two = add_node(automata, FALSE, 1);
            node_three = aux_regex_to_automata(*regex.one, automata, node_two);
            end_node = add_node(automata, TRUE, 2);

            node_three->success = FALSE;

            arrow = add_arrow(start_node, LABEL_EPSILON, node_one->num, 1);
            arrow->counters_actions[0] = new_counter_action(0, ACTION_SET, 0);

            arrow = add_arrow(node_one, LABEL_EPSILON, node_two->num, 1);
            // regex.max - 1, because that AT_MOST is inclusive
            // we must not follow the arrow if we already reached regex.max
            arrow->counters_actions[0] = new_counter_action(0, ACTION_AT_MOST, regex.max - 1);

            arrow = add_arrow(node_three, LABEL_EPSILON, node_one->num, 1);
            arrow->counters_actions[0] = new_counter_action(0, ACTION_ADD, 1);

            arrow = add_arrow(node_one, LABEL_EPSILON, end_node->num, 2);
            arrow->counters_actions[0] = new_counter_action(0, ACTION_AT_LEAST, regex.min);
            arrow->counters_actions[1] = new_counter_action(0, ACTION_AT_MOST, regex.max);
        break;
    }
    return end_node;
}

void aux_regex_to_string (Regex regex, String_Builder * builder) {
    char buffer [100];
    switch (regex.tag_regex) {
        case REGEX_EPSILON:
            append(builder, "EPS");
        break;
        case REGEX_CHARACTER:
            sprintf(buffer, "%i", regex.character);
            append(builder, buffer);
        break;
        case REGEX_SEQUENCE:
            aux_regex_to_string(*regex.one, builder);
            append(builder, ".");
            aux_regex_to_string(*regex.two, builder);
        break;
        case REGEX_BRANCH:
            append(builder, "(");
            aux_regex_to_string(*regex.one, builder);
            append(builder, "|");
            aux_regex_to_string(*regex.two, builder);
            append(builder, ")");
        break;
        case REGEX_REPEAT:
            append(builder, "(");
            aux_regex_to_string(*regex.one, builder);
            sprintf(buffer, "){%i,%i}", regex.min, regex.max);
            append(builder, buffer);
        break;
    }
}

char * regex_to_string (Regex regex) {
    String_Builder builder = new_string_builder(16);
    aux_regex_to_string (regex, &builder);
    return builder.str;
}
