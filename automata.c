#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "automata.h"

enum { FALSE = 0, TRUE = 1 }; // used only for assignment

Pipe new_pipe ();
void push (Pipe * pipe_walk, Walk walk);

int apply_counters_actions (Walk walk, Arrow arrow);

/*int main (char ** argv) {

	Automata automata = new_automata(1, 1);
	Node * node0 = add_node(&automata, TRUE, 2);
	Arrow * arrow0 = add_arrow(node0, 0, 0, 2);
	arrow0->counters_actions[0] = new_counter_action(0, ACTION_ADD, 1);
	arrow0->counters_actions[1] = new_counter_action(0, ACTION_AT_MOST, 20);
	Arrow * arrow1 = add_arrow(node0, 0, 0, 0);

	int labels[100] = { 0 };
	Pipe success_walks = all_success_walks(automata, 0, 22, labels);

	int longest_nb_labels_used = 0;
	int longest_walk_node_num = 0; // 0 is the initial node

	while (success_walks.length > 0) {
		Walk walk = pop(&success_walks);
		if (walk.nb_labels_used > longest_nb_labels_used) {
			longest_nb_labels_used = walk.nb_labels_used;
			longest_walk_node_num = walk.node->num;
		}
		free_walk(walk);
	}

	printf("%i labels used, landed in node %i\n", longest_nb_labels_used, longest_walk_node_num);

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
Node * add_node (Automata * automata, int success, int nb_arrows_capacity) {

	automata->nb_nodes ++;

	if (automata->nb_nodes > automata->nb_nodes_capacity) {

		if (automata->nb_nodes_capacity == 0) automata->nb_nodes_capacity = 1;
		else (automata->nb_nodes_capacity *= 2);

		automata->nodes = realloc(automata->nodes, automata->nb_nodes_capacity * sizeof(Node));
	}

	Node node;
	node.num = automata->nb_nodes - 1;
	node.success = success;
	node.nb_arrows = 0;
	node.nb_arrows_capacity = nb_arrows_capacity;
	node.arrows = malloc (nb_arrows_capacity * sizeof(Arrow));

	automata->nodes[node.num] = node;

	return &automata->nodes[node.num];
}

Arrow * add_arrow (Node * node, int label, int dest, int nb_counters_actions) {

	int num_arrow = node->nb_arrows ++;

	if (node->nb_arrows > node->nb_arrows_capacity) {

		if (node->nb_arrows_capacity == 0) node->nb_arrows_capacity = 1;
		else node->nb_arrows_capacity *= 2;
		node->arrows = realloc(node->arrows, node->nb_arrows_capacity * sizeof(Arrow));
	}

	Arrow arrow;
	arrow.label = label;
	arrow.dest = dest;
	arrow.nb_counters_actions = nb_counters_actions;
	arrow.counters_actions = malloc (nb_counters_actions * sizeof(Counter_Action));

	node->arrows[num_arrow] = arrow;

	return &node->arrows[num_arrow];
}

// utility function to build structure in one line
Counter_Action new_counter_action (int num_counter, int action, int action_param) {
    Counter_Action counter_action = { num_counter, action, action_param };
    return counter_action;
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
Pipe all_success_walks (Automata automata, int start_node_num, int nb_labels, int * labels) {

	Walk first_walk;
	first_walk.nb_labels_used = 0;
	first_walk.node = &automata.nodes[start_node_num];
	first_walk.counters = malloc(automata.nb_counters * sizeof(int));
	for (int i = 0; i < automata.nb_counters; i ++) {
		first_walk.counters[i] = 0;
	}

	Pipe walks_to_go = new_pipe();
	push(&walks_to_go, first_walk);

	Pipe success_walks = new_pipe();

	while (walks_to_go.length > 0) {

		Walk walk = pop(&walks_to_go);

		if (walk.node->success) {
			Walk success_walk = copy_walk(walk, automata.nb_counters);
			push(&success_walks, success_walk);
		}

		if (walk.nb_labels_used < nb_labels) {

			int next_label = labels[walk.nb_labels_used];

			for (int i = 0; i < walk.node->nb_arrows; i ++) {
				Arrow arrow = walk.node->arrows[i];

				// follow the arrow if the labels are equal
				if (arrow.label == next_label) {
					Walk next_walk = copy_walk(walk, automata.nb_counters);
					int success = apply_counters_actions(next_walk, arrow);
					if (success) {
						next_walk.nb_labels_used ++;
						next_walk.node = &automata.nodes[arrow.dest];
						push(&walks_to_go, next_walk);
					}
					else free_walk(next_walk);
				}
				// or follow the epsilon-transition
				else if (arrow.label == LABEL_EPSILON) {
					Walk next_walk = copy_walk(walk, automata.nb_counters);
					int success = apply_counters_actions(next_walk, arrow);
					if (success) {
						next_walk.node = &automata.nodes[arrow.dest];
						push(&walks_to_go, next_walk);
					}
					else free_walk(next_walk);
				}
			}
		}
		else {
			for (int i = 0; i < walk.node->nb_arrows; i ++) {
				Arrow arrow = walk.node->arrows[i];

				if (arrow.label == LABEL_EPSILON) {
					Walk next_walk = copy_walk(walk, automata.nb_counters);
					int success = apply_counters_actions(next_walk, arrow);
					if (success) {
						next_walk.node = &automata.nodes[arrow.dest];
						push(&walks_to_go, next_walk);
					}
					else free_walk(next_walk);
				}
			}
		}

		free_walk(walk);
	}

	return success_walks;
}

void longest_success_walk (
	Automata automata, int start_node_num, int nb_labels, int * labels,
	int * nb_labels_used, int * end_node_num
) {
	Pipe success_walks = all_success_walks(automata, start_node_num, nb_labels, labels);

	*nb_labels_used = 0;
	*end_node_num = 0;

	while (success_walks.length > 0) {
		Walk walk = pop(&success_walks);
		if (walk.nb_labels_used > *nb_labels_used) {
			*nb_labels_used = walk.nb_labels_used;
			*end_node_num = walk.node->num;
		}
		free_walk(walk);
	}
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

Pipe new_pipe () {
	Pipe pipe_walk;
	pipe_walk.length = 0;
	return pipe_walk;
}

// pipe_walk must dereference
// node must dereference
void push (Pipe * pipe_walk, Walk walk) {

	// put the walk in a list_walk (on the heap)
	List * list_walk = malloc(sizeof(List));
	list_walk->walk = walk;

	pipe_walk->length ++;

	if (pipe_walk->length == 1) {
		pipe_walk->first = list_walk;
	}
	else if (pipe_walk->length == 2) {
		pipe_walk->first->next = list_walk;
		pipe_walk->last = list_walk;
	}
	else {
		// replacing the current last
		pipe_walk->last->next = list_walk;
		pipe_walk->last = list_walk;
	}
}

// pipe_walk must have at least one element
// return NULL when pipe_walk has zero elements
// return pipe_walk.first otherwise
Walk pop (Pipe * pipe_walk) {

	// memo the list_walk and the walk
	List * popped_list_walk = pipe_walk->first;
	Walk popped_walk = popped_list_walk->walk;

	// pop the list_walk
	pipe_walk->length --;
	pipe_walk->first = pipe_walk->first->next;

	free(popped_list_walk);

	return popped_walk;
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

			if (node.success) {
				sprintf(buffer, "((%i))", node.num);
			}
			else {
				sprintf(buffer, "(%i)", node.num);
			}
			append(&builder, buffer);

			if (arrow.label == LABEL_EPSILON) {
				append(&builder, "--EPS-->");
			}
			else {
				sprintf(buffer, "--%i-->", arrow.label);
				append(&builder, buffer);
			}

			Node dest_node = automata.nodes[arrow.dest];
			if (dest_node.success) {
				sprintf(buffer, "((%i))", dest_node.num);
			}
			else {
				sprintf(buffer, "(%i)", dest_node.num);
			}
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

			int hasLabels = arrow.label != LABEL_EPSILON || arrow.nb_counters_actions > 0;

			if (hasLabels) append(&builder, " [label=\"");

			if (arrow.label != LABEL_EPSILON) {
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

			if (hasLabels) append(&builder, "\"]");
			append(&builder, ";\n");
		}
		append(&builder, "\n");
	}

	append(&builder, "}\n");

	return builder.str;
}
