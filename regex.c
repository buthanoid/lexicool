#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "automata.h"
#include "regex.h"

// some US-ASCII regex
Regex ascii_digit = { REGEX_INTERVAL, 0, NULL, NULL, 48, 57 };
Regex ascii_letter_small = { REGEX_INTERVAL, 0, NULL, NULL, 97, 122 };
Regex ascii_letter_cap = { REGEX_INTERVAL, 0, NULL, NULL, 65, 90 };
Regex ascii_letter_all = { REGEX_BRANCH, 0, &ascii_letter_small, &ascii_letter_cap, 0, 0 };

/*int main (int argc, char ** argv) {

    Regex regex1;
    regex1.type = REGEX_REPEAT;
    regex1.min = 2;
    regex1.max = 2;
    regex1.one = &ascii_letter_all;

    Regex regex2;
    regex2.type = REGEX_SEQUENCE;
    regex2.one = &regex1;
    regex2.two = &ascii_digit;

    Regex regex3;
    regex3.type = REGEX_REPEAT;
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

    int labels[] = { 'a', 'b', '1', 'c', 'd', '2', 'e', 'f', '3', 'g', 'h', '4' };
    int nb_labels = 12;

    int res_num_node, res_nb_labels_used;
	int * res_counters = malloc(automata.nb_counters * sizeof(int));
	int res_nb_explorations_steps, res_max_nb_points_reached;

    explore_farthest_success_node(
        automata, 0, labels, nb_labels,
        &res_num_node, &res_nb_labels_used, res_counters,
        &res_nb_explorations_steps, &res_max_nb_points_reached);

    printf("nb explorations steps: %i, max nb points reached: %i\n",
        res_nb_explorations_steps, res_max_nb_points_reached);

    printf("final node num: %i, nb labels used: %i/%i\n",
        res_num_node, res_nb_labels_used, nb_labels);

    printf("final counters: [");
    for (int i = 0; i < automata.nb_counters; i ++) printf("%i,", res_counters[i]);
    printf("]\n\n");

    free(res_counters);
    free_automata(automata);
}*/

void aux_length_regex_to_automata (Regex regex, int * nb_nodes, int * nb_counters);
int aux_regex_to_automata (Regex regex, Automata * automata, int start_node, int * nb_counters);

Automata regex_to_automata (Regex regex) {

    int nb_nodes = 0, nb_counters = 0;
    aux_length_regex_to_automata(regex, &nb_nodes, &nb_counters);

    Automata automata;
    new_automata(&automata, nb_nodes, nb_counters);

    int init_node = add_node(&automata, FALSE, 2);

    int nb_counters_used = 0;
    aux_regex_to_automata(regex, &automata, init_node, &nb_counters_used);

    return automata;
}

void aux_length_regex_to_automata (Regex regex, int * nb_nodes, int * nb_counters) {
    switch (regex.type) {
        case REGEX_EPSILON:
        case REGEX_CHARACTER:
        case REGEX_INTERVAL:
            // two nodes with an arrow
            // (0)--a-->(1)
            *nb_nodes += 2;
        break;
        case REGEX_SEQUENCE:
        case REGEX_BRANCH:
            aux_length_regex_to_automata(*regex.one, nb_nodes, nb_counters);
            aux_length_regex_to_automata(*regex.two, nb_nodes, nb_counters);
            if (regex.type == REGEX_SEQUENCE) (*nb_nodes) --;
        break;
        case REGEX_REPEAT:
            (*nb_counters) ++;
            aux_length_regex_to_automata(*regex.one, nb_nodes, nb_counters);
            *nb_nodes += 4;
        break;
    }
}

int aux_regex_to_automata (
    Regex regex, Automata * automata, int start_node, int * nb_counters_used
) {
    int end_node;
    int node_one, node_two, node_three;
    int arrow;
    int num_counter;
    switch (regex.type) {
        case REGEX_EPSILON:
            end_node = add_node(automata, TRUE, 2);
            add_epsilon_arrow(automata, start_node, end_node, 0);
        break;
        case REGEX_CHARACTER:
            // wet set arrow capacity to 2 because we will never need more than 2 arrows
            // we choose to allocate largest possible, rather than to allocate often
            end_node = add_node(automata, TRUE, 2);
            add_label_arrow(automata, start_node, regex.character, end_node, 0);
        break;
        case REGEX_SEQUENCE:
            node_one = aux_regex_to_automata(*regex.one, automata, start_node, nb_counters_used);
            end_node = aux_regex_to_automata(*regex.two, automata, node_one, nb_counters_used);
            automata->nodes[node_one].success = FALSE;
        break;
        case REGEX_BRANCH:
            node_one = aux_regex_to_automata(*regex.one, automata, start_node, nb_counters_used);
            node_two = aux_regex_to_automata(*regex.two, automata, start_node, nb_counters_used);
            end_node = add_node(automata, TRUE, 2);
            automata->nodes[node_one].success = FALSE;
            automata->nodes[node_two].success = FALSE;
            add_epsilon_arrow(automata, node_one, end_node, 0);
            add_epsilon_arrow(automata, node_two, end_node, 0);
        break;
        case REGEX_REPEAT:
            num_counter = (*nb_counters_used) ++;
            node_one = add_node(automata, FALSE, 2);
            node_two = add_node(automata, FALSE, 1);
            node_three = aux_regex_to_automata(*regex.one, automata, node_two, nb_counters_used);
            end_node = add_node(automata, TRUE, 2);

            automata->nodes[node_three].success = FALSE;

            arrow = add_epsilon_arrow(automata, start_node, node_one, 1);
            add_counter_action(automata, start_node, arrow, num_counter, ACTION_SET, 0);

            if (regex.max == MAX_INF) {
                add_epsilon_arrow(automata, node_one, node_two, 0);
            }
            else {
                arrow = add_epsilon_arrow(automata, node_one, node_two, 1);
                // regex.max - 1, because AT_MOST is inclusive
                // we must not follow the arrow if we already reached regex.max
                add_counter_action(
                    automata, node_one, arrow, num_counter, ACTION_AT_MOST, regex.max - 1);
            }

            arrow = add_epsilon_arrow(automata, node_three, node_one, 1);
            add_counter_action(automata, node_three, arrow, num_counter, ACTION_ADD, 1);

            arrow = add_epsilon_arrow(automata, node_one, end_node, (regex.max == MAX_INF) ? 1 : 2);

            add_counter_action(
                automata, node_one, arrow, num_counter, ACTION_AT_LEAST, regex.min);

            if (regex.max != MAX_INF) {
                add_counter_action(
                    automata, node_one, arrow, num_counter, ACTION_AT_MOST, regex.max);
            }
        break;
        case REGEX_INTERVAL:
            end_node = add_node(automata, TRUE, 2);
            add_interval_arrow(automata, start_node, regex.min, regex.max, end_node, 0);
        break;
    }
    return end_node;
}

void aux_regex_to_string (Regex regex, String_Builder * builder) {
    char buffer [100];
    switch (regex.type) {
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
        case REGEX_INTERVAL:
            sprintf(buffer, "[%i-%i]", regex.min, regex.max);
            append(builder, buffer);
        break;
    }
}

char * regex_to_string (Regex regex) {
    String_Builder builder = new_string_builder(16);
    aux_regex_to_string (regex, &builder);
    return builder.chars;
}
