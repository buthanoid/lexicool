#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "automata.h"

enum { FALSE = 0, TRUE = 1 };

enum { MAX_INF = -1 };

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
void aux_length_regex_to_automata (Regex regex, int * nb_nodes, int * nb_counters);
Node * aux_regex_to_automata (
    Regex regex, Automata * automata, Node * start_node, int * nb_counters);

char * regex_to_string (Regex regex);

int main (char ** argv) {

    Regex regex0;
    regex0.tag_regex = REGEX_CHARACTER;
    regex0.character = 12;

    Regex regex4;
    regex4.tag_regex = REGEX_REPEAT;
    regex4.min = 2;
    regex4.max = 2;
    regex4.one = &regex0;

    Regex regex1;
    regex1.tag_regex = REGEX_CHARACTER;
    regex1.character = 4000;

    Regex regex2;
    regex2.tag_regex = REGEX_SEQUENCE;
    regex2.one = &regex4;
    regex2.two = &regex1;

    Regex regex3;
    regex3.tag_regex = REGEX_REPEAT;
    regex3.min = 1;
    regex3.max = MAX_INF;
    regex3.one = &regex2;

    char * string_regex = regex_to_string(regex3);
    printf("%s\n\n", string_regex);
    free(string_regex);

    Automata automata = regex_to_automata(regex3);

    char * string_automata = automata_to_string(automata);
    printf("%s\n", string_automata);
    free(string_automata);

    int labels[] = { 12, 12, 4000, 12, 12, 4000, 12, 12, 4000, 12, 12, 4000 };

    int nb_labels_used, end_node_num;
    longest_success_walk(automata, 0, 12, labels, &nb_labels_used, &end_node_num);

    printf("%i labels used, landed in node %i\n", nb_labels_used, end_node_num);

    free_automata(automata);
}


Automata regex_to_automata (Regex regex) {

    int nb_nodes = 0, nb_counters = 0;
    aux_length_regex_to_automata(regex, &nb_nodes, &nb_counters);

    Automata automata = new_automata(nb_nodes, nb_counters);

    Node * init_node = add_node(&automata, FALSE, 2);

    int nb_counters_used = 0;
    aux_regex_to_automata(regex, &automata, init_node, &nb_counters_used);

    return automata;
}

void aux_length_regex_to_automata (Regex regex, int * nb_nodes, int * nb_counters) {
    int result_length, result_one, result_two;
    switch (regex.tag_regex) {
        case REGEX_EPSILON:
        case REGEX_CHARACTER:
            // two nodes with an arrow
            // (0)--a-->(1)
            *nb_nodes += 2;
        break;
        case REGEX_SEQUENCE:
        case REGEX_BRANCH:
            aux_length_regex_to_automata(*regex.one, nb_nodes, nb_counters);
            aux_length_regex_to_automata(*regex.two, nb_nodes, nb_counters);
            if (regex.tag_regex == REGEX_SEQUENCE) (*nb_nodes) --;
        break;
        case REGEX_REPEAT:
            (*nb_counters) ++;
            aux_length_regex_to_automata(*regex.one, nb_nodes, nb_counters);
            *nb_nodes += 4;
        break;
    }
}

Node * aux_regex_to_automata (
    Regex regex, Automata * automata, Node * start_node, int * nb_counters_used
) {
    Node * end_node, * node_one, * node_two, * node_three;
    Arrow * arrow;
    int num_counter;
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
            node_one = aux_regex_to_automata(*regex.one, automata, start_node, nb_counters_used);
            end_node = aux_regex_to_automata(*regex.two, automata, node_one, nb_counters_used);
            node_one->success = FALSE;
        break;
        case REGEX_BRANCH:
            node_one = aux_regex_to_automata(*regex.one, automata, start_node, nb_counters_used);
            node_two = aux_regex_to_automata(*regex.two, automata, start_node, nb_counters_used);
            end_node = add_node(automata, TRUE, 2);
            node_one->success = FALSE;
            node_two->success = FALSE;
            add_arrow(node_one, LABEL_EPSILON, end_node->num, 0);
            add_arrow(node_two, LABEL_EPSILON, end_node->num, 0);
        break;
        case REGEX_REPEAT:
            num_counter = (*nb_counters_used) ++;
            node_one = add_node(automata, FALSE, 2);
            node_two = add_node(automata, FALSE, 1);
            node_three = aux_regex_to_automata(*regex.one, automata, node_two, nb_counters_used);
            end_node = add_node(automata, TRUE, 2);

            node_three->success = FALSE;

            arrow = add_arrow(start_node, LABEL_EPSILON, node_one->num, 1);
            arrow->counters_actions[0] = new_counter_action(num_counter, ACTION_SET, 0);

            if (regex.max == MAX_INF) {
                arrow = add_arrow(node_one, LABEL_EPSILON, node_two->num, 0);
            }
            else {
                arrow = add_arrow(node_one, LABEL_EPSILON, node_two->num, 1);
                // regex.max - 1, because that AT_MOST is inclusive
                // we must not follow the arrow if we already reached regex.max
                arrow->counters_actions[0] =
                    new_counter_action(num_counter, ACTION_AT_MOST, regex.max - 1);
            }

            arrow = add_arrow(node_three, LABEL_EPSILON, node_one->num, 1);
            arrow->counters_actions[0] = new_counter_action(num_counter, ACTION_ADD, 1);

            arrow = add_arrow(node_one, LABEL_EPSILON, end_node->num,
                (regex.max == MAX_INF) ? 1 : 2);

            arrow->counters_actions[0] =
                new_counter_action(num_counter, ACTION_AT_LEAST, regex.min);

            if (regex.max != MAX_INF) {
                arrow->counters_actions[1] =
                    new_counter_action(num_counter, ACTION_AT_MOST, regex.max);
            }
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
            sprintf(buffer, "){%i,", regex.min);
            append(builder, buffer);
            if (regex.max == MAX_INF) append(builder, "*}");
            else {
                sprintf(buffer, "%i}", regex.max);
                append(builder, buffer);
            }
        break;
    }
}

char * regex_to_string (Regex regex) {
    String_Builder builder = new_string_builder(16);
    aux_regex_to_string (regex, &builder);
    return builder.str;
}
