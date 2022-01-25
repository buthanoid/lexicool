#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "automata.h"

int apply_counters_actions (Walk walk, Arrow arrow);

/*int main (int argc, char ** argv) {

	Automata automata = new_automata(1, 1);
	int node0 = add_node(&automata, TRUE, 2);
	int arrow0 = add_label_arrow(&automata, node0, 0, 0, 2);
	add_counter_action(&automata, node0, arrow0, 0, ACTION_ADD, 1);
	add_counter_action(&automata, node0, arrow0, 0, ACTION_AT_MOST, 20);
	add_label_arrow(&automata, node0, 0, 0, 0);

	int labels[100] = { 0 };
	int nb_labels_used, end_node_num;
	longest_success_walk(automata, 0, 25, labels, &nb_labels_used, &end_node_num);

	printf("%i labels used, landed in node %i\n", nb_labels_used, end_node_num);

	char * string_automata = automata_to_string(automata);
	printf("%s\n", string_automata);
	free(string_automata);

	free_automata(automata);
}*/

// nb_nodes must be > 0
Automata new_automata (int nb_nodes_capacity, int nb_counters) {
	Automata automata;
	automata.nb_nodes = 0;
	automata.nb_nodes_capacity = nb_nodes_capacity;
	automata.nodes = malloc (nb_nodes_capacity * sizeof(Node));
	automata.nb_counters = nb_counters;
	return automata;
}

// num must be >= 0
// success must be boolean
// nb_arrows must be >= 0
int add_node (Automata * automata, int success, int nb_arrows_capacity) {

	int success_adapt = adapt_capacity(
		&automata->nb_nodes_capacity, (void **) &automata->nodes,
		automata->nb_nodes + 1, sizeof(Node));

	if (success_adapt) {
		int num_node = automata->nb_nodes ++;

		Node node;
		node.num = num_node;
		node.success = success;
		node.nb_arrows = 0;
		node.nb_arrows_capacity = nb_arrows_capacity;
		node.arrows = malloc (nb_arrows_capacity * sizeof(Arrow));

		automata->nodes[num_node] = node;

		return num_node;
	}
	else return -1;
}

int add_arrow (
	Automata * automata, int num_node, int label, int epsilon,
	int dest, int nb_counters_actions_capacity
) {

	Node * node = &automata->nodes[num_node];

	int success_adapt = adapt_capacity(
		&node->nb_arrows_capacity, (void **) &node->arrows, node->nb_arrows + 1, sizeof(Arrow));

	if (success_adapt) {
		int num_arrow = node->nb_arrows ++;

		Arrow arrow;
		arrow.label = label;
		arrow.epsilon = epsilon;
		arrow.dest = dest;
		arrow.nb_counters_actions = 0;
		arrow.nb_counters_actions_capacity = nb_counters_actions_capacity;
		arrow.counters_actions = malloc (nb_counters_actions_capacity * sizeof(Counter_Action));

		node->arrows[num_arrow] = arrow;

		return num_arrow;
	}
	else return -1;
}

int add_label_arrow (
	Automata * automata, int num_node, int label, int dest, int nb_counters_actions_capacity
) {
	return add_arrow(automata, num_node, label, FALSE, dest, nb_counters_actions_capacity);
}

int add_epsilon_arrow (
	Automata * automata, int num_node, int dest, int nb_counters_actions_capacity
) {
	return add_arrow(automata, num_node, 0, TRUE, dest, nb_counters_actions_capacity);
}

int add_counter_action (
	Automata * automata, int num_node, int num_arrow, int num_counter, int action, int action_param
) {
	Node node = automata->nodes[num_node];
	Arrow * arrow = &node.arrows[num_arrow];

	int success_adapt = adapt_capacity(
		&arrow->nb_counters_actions_capacity, (void **) &arrow->counters_actions,
		arrow->nb_counters_actions + 1, sizeof(Counter_Action));

	if (success_adapt) {
		int num_counter_action = arrow->nb_counters_actions ++;

	    Counter_Action counter_action = { num_counter, action, action_param };
		arrow->counters_actions[num_counter_action] = counter_action;

		return num_counter_action;
	}
	else return -1;
}

Walk copy_walk (Walk walk, int nb_counters) {
	Walk new_walk = walk;
	new_walk.counters = malloc(nb_counters * sizeof(int));
	for (int i = 0; i < nb_counters; i ++) {
		new_walk.counters[i] = walk.counters[i];
	}
	return new_walk;
}

// automata.nb_nodes must be > 0
// start_node_num must be >= 0
// start_node_num must be < automata.nb_nodes
// nb_labels must be >= 0
// return can have zero elements (return.first == NULL)
Stack all_success_walks (Automata automata, int start_node_num, int nb_labels, int * labels) {
	int explored = 0;
	Walk first_walk;
	first_walk.nb_labels_used = 0;
	first_walk.node_num = start_node_num;
	first_walk.counters = malloc(automata.nb_counters * sizeof(int));
	for (int i = 0; i < automata.nb_counters; i ++) {
		first_walk.counters[i] = 0;
	}

	Stack stack_remaining = { 0, 0, NULL };
	push(first_walk, &stack_remaining);

	Stack stack_success = { 0, 0, NULL };

	while (stack_remaining.nb_walks > 0) {
		explored ++;
		Walk walk = pop(&stack_remaining);
		Node walk_node = automata.nodes[walk.node_num];

		if (walk_node.success) {
			Walk success_walk = copy_walk(walk, automata.nb_counters);
			push(success_walk, &stack_success);
		}

		// even if has_next_label is false, we still need to follow epsilon arrows
		int next_label;
		int has_next_label = (walk.nb_labels_used < nb_labels);
		if (has_next_label) next_label = labels[walk.nb_labels_used];

		for (int i = 0; i < walk_node.nb_arrows; i ++) {
			Arrow arrow = walk_node.arrows[i];

			if ((has_next_label && next_label == arrow.label) || arrow.epsilon) {

				Walk new_walk = copy_walk(walk, automata.nb_counters);

				int success_actions = apply_counters_actions(new_walk, arrow);
				if (success_actions) {

					if (! arrow.epsilon) new_walk.nb_labels_used ++;
					new_walk.node_num = arrow.dest;

					push(new_walk, &stack_remaining);
				}
				else free_walk(new_walk);
			}
		}

		free_walk(walk);
	}
	printf("explored %i walks\n", explored);
	free(stack_remaining.walks);
	return stack_success;
}

void longest_success_walk (
	Automata automata, int start_node_num, int nb_labels, int * labels,
	int * nb_labels_used, int * end_node_num
) {
	Stack stack_success = all_success_walks(automata, start_node_num, nb_labels, labels);

	*nb_labels_used = 0;
	*end_node_num = start_node_num;

	while (stack_success.nb_walks > 0) {
		Walk walk = pop(&stack_success);
		if (walk.nb_labels_used > *nb_labels_used) {
			*nb_labels_used = walk.nb_labels_used;
			*end_node_num = walk.node_num;
		}
		free_walk(walk);
	}

	free(stack_success.walks);
}

int apply_counters_actions (Walk walk, Arrow arrow) {
	int success = TRUE;

	for (int i = 0; i < arrow.nb_counters_actions; i ++) {

		Counter_Action counter_action = arrow.counters_actions[i];
		int num_counter = counter_action.num_counter;
		int param = counter_action.action_param;

		switch (counter_action.action) {
			case ACTION_SET:
				walk.counters[num_counter] = param;
				break;
			case ACTION_ADD:
				walk.counters[num_counter] += param;
				break;
			case ACTION_AT_LEAST:
				if (walk.counters[num_counter] < param) {
					success = FALSE;
					i = arrow.nb_counters_actions; // end loop
				}
				break;
			case ACTION_AT_MOST:
				if (walk.counters[num_counter] > param) {
					success = FALSE;
					i = arrow.nb_counters_actions; // end loop
				}
				break;
		}
	}

	return success;
}

void push (Walk walk, Stack * stack) {
	int success_adapt = adapt_capacity(
		&stack->nb_walks_capacity, (void **) &stack->walks, stack->nb_walks + 1, sizeof(Walk));
	if (success_adapt) {
		stack->walks[stack->nb_walks] = walk;
		stack->nb_walks ++;
	}
}

Walk pop (Stack * stack) {
	stack->nb_walks --;
	return stack->walks[stack->nb_walks];
}

void free_automata (Automata automata) {
	for (int i = 0; i < automata.nb_nodes; i ++) {
		Node node = automata.nodes[i];
		for (int j = 0; j < node.nb_arrows; j ++) {
			Arrow arrow = node.arrows[j];
			free(arrow.counters_actions);
		}
		free(node.arrows);
	}
	free(automata.nodes);
}

void free_walk (Walk walk) {
	free(walk.counters);
}

char * automata_to_string (Automata automata) {
	String_Builder builder = new_string_builder(automata.nb_nodes * 10);
	char buffer [100];

	sprintf(buffer, "automata %i nodes %i counters\n", automata.nb_nodes, automata.nb_counters);
	append(&builder, buffer);

	for (int i = 0; i < automata.nb_nodes; i ++) {
		Node node = automata.nodes[i];
		for (int j = 0; j < node.nb_arrows; j++) {
			Arrow arrow = node.arrows[j];

			if (node.success) { sprintf(buffer, "((%i))", node.num); }
			else { sprintf(buffer, "(%i)", node.num); }
			append(&builder, buffer);

			if (arrow.epsilon) { append(&builder, "--EPS-->"); }
			else {
				sprintf(buffer, "--%i-->", arrow.label);
				append(&builder, buffer);
			}

			Node dest_node = automata.nodes[arrow.dest];
			if (dest_node.success) { sprintf(buffer, "((%i))", dest_node.num); }
			else { sprintf(buffer, "(%i)", dest_node.num); }
			append(&builder, buffer);

			for (int k = 0; k < arrow.nb_counters_actions; k ++) {
				Counter_Action counter_action = arrow.counters_actions[k];
				switch (counter_action.action) {
					case ACTION_SET:
						sprintf(buffer, " [c%i := %i]",
							counter_action.num_counter, counter_action.action_param);
					break;
					case ACTION_ADD:
						sprintf(buffer, " [c%i += %i]",
							counter_action.num_counter, counter_action.action_param);
					break;
					case ACTION_AT_LEAST:
						sprintf(buffer, " [c%i >= %i]",
							counter_action.num_counter, counter_action.action_param);
					break;
					case ACTION_AT_MOST:
						sprintf(buffer, " [c%i <= %i]",
							counter_action.num_counter, counter_action.action_param);
					break;
				}
				append(&builder, buffer);
			}

			append(&builder, "\n");
		}
	}

	return builder.str;
}

char * automata_to_dot (Automata automata) {
	String_Builder builder = new_string_builder(automata.nb_nodes * 10);
	char buffer [100];

	append(&builder, "digraph automata {\n\n");

	for (int i = 0; i < automata.nb_nodes; i ++) {
		Node node = automata.nodes[i];

		sprintf(buffer, "node%i [label=\"%i\"%s];\n",
			node.num, node.num, node.success ? ", peripheries=2" : "");
		append(&builder, buffer);

		for (int j = 0; j < node.nb_arrows; j++) {
			Arrow arrow = node.arrows[j];

			sprintf(buffer, "node%i -> node%i", node.num, arrow.dest);
			append(&builder, buffer);

			int hasDotLabel = (! arrow.epsilon) || arrow.nb_counters_actions > 0;

			if (hasDotLabel) append(&builder, " [label=\"");

			if (! arrow.epsilon) {
				sprintf(buffer, "%i", arrow.label);
				append(&builder, buffer);
			}

			for (int k = 0; k < arrow.nb_counters_actions; k ++) {
				Counter_Action counter_action = arrow.counters_actions[k];
				switch (counter_action.action) {
					case ACTION_SET:
						sprintf(buffer, " [c%i := %i]",
							counter_action.num_counter, counter_action.action_param);
					break;
					case ACTION_ADD:
						sprintf(buffer, " [c%i += %i]",
							counter_action.num_counter, counter_action.action_param);
					break;
					case ACTION_AT_LEAST:
						sprintf(buffer, " [c%i >= %i]",
							counter_action.num_counter, counter_action.action_param);
					break;
					case ACTION_AT_MOST:
						sprintf(buffer, " [c%i <= %i]",
							counter_action.num_counter, counter_action.action_param);
					break;
				}
				append(&builder, buffer);
			}

			if (hasDotLabel) append(&builder, "\"]");
			append(&builder, ";\n");
		}
		append(&builder, "\n");
	}

	append(&builder, "}\n");

	return builder.str;
}
