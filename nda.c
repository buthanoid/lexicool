#include <stdlib.h>
#include <stdio.h>

#include "nda.h"

enum { FALSE, TRUE };

typedef struct {
	int nb_labels_used;
	int * counters;
	Node * node;
} Walk;

typedef struct List {
	Walk walk;
	struct List * next;
} List;

typedef struct {
	int length;
	List * first;
	List * last;
} Pipe;

Pipe create_Pipe ();
void push (Pipe * pipe_walk, Walk walk);
Walk pop (Pipe * pipe_walk);

int apply_counters_actions (Walk walk, Arrow arrow);
Pipe all_success_walks (NDA nda, int start_node_num, int nb_labels, int * labels);

/*int main (char ** argv) {

	NDA nda = create_NDA(1, 1);
	Node * node0 = add_Node(&nda, TRUE, 2);
	Arrow * arrow0 = add_arrow(node0, 0, 0, 2);
	arrow0->counters_actions[0] = new_counter_action(0, ACTION_ADD, 1);
	arrow0->counters_actions[1] = new_counter_action(0, ACTION_AT_MOST, 20);
	Arrow * arrow1 = add_arrow(node0, 0, 0, 0);

	int labels[100] = { 0 };
	Pipe success_walks = all_success_walks(nda, 0, 22, labels);

	int longest_nb_labels_used = 0;
	int longest_walk_node_num = 0; // 0 is the initial node

	while (success_walks.length > 0) {
		Walk walk = pop(&success_walks);
		if (walk.nb_labels_used > longest_nb_labels_used) {
			longest_nb_labels_used = walk.nb_labels_used;
			longest_walk_node_num = walk.node->num;
		}
		free(walk.counters);
	}
	printf("%i %i %s\n",
		longest_nb_labels_used, longest_walk_node_num,
		(nda.nodes[longest_walk_node_num].success == TRUE) ? "success" : "failure");
}*/

// nb_nodes must be > 0
NDA create_NDA (int nb_nodes_capacity, int nb_counters) {
	NDA nda;
	nda.nb_nodes = 0;
	nda.nb_nodes_capacity = nb_nodes_capacity;
	nda.nodes = malloc (nb_nodes_capacity * sizeof(Node));
	nda.nb_counters = nb_counters;
	return nda;
}

// num must be >= 0
// success must be boolean
// nb_arrows must be >= 0
Node * add_Node (NDA * nda, int success, int nb_arrows_capacity) {

	nda->nb_nodes ++;

	if (nda->nb_nodes > nda->nb_nodes_capacity) {

		if (nda->nb_nodes_capacity == 0) nda->nb_nodes_capacity = 1;
		else (nda->nb_nodes_capacity *= 2);

		nda->nodes = realloc(nda->nodes, nda->nb_nodes_capacity * sizeof(Node));
	}

	Node node;
	node.num = nda->nb_nodes - 1;
	node.success = success;
	node.nb_arrows = 0;
	node.nb_arrows_capacity = nb_arrows_capacity;
	node.arrows = malloc (nb_arrows_capacity * sizeof(Arrow));

	nda->nodes[node.num] = node;

	return &nda->nodes[node.num];
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

// nda.nb_nodes must be > 0
// start_node_num must be >= 0
// start_node_num must be < nda.nb_nodes
// nb_labels must be >= 0
// return can have zero elements (return.first == NULL)
Pipe all_success_walks (NDA nda, int start_node_num, int nb_labels, int * labels) {

	Walk first_walk;
	first_walk.nb_labels_used = 0;
	first_walk.node = &nda.nodes[start_node_num];
	first_walk.counters = malloc(nda.nb_counters * sizeof(int));
	for (int i = 0; i < nda.nb_counters; i ++) {
		first_walk.counters[i] = 0;
	}

	Pipe walks_to_go = create_Pipe();
	push(&walks_to_go, first_walk);

	Pipe success_walks = create_Pipe();

	while (walks_to_go.length > 0) {

		Walk walk = pop(&walks_to_go);

		if (walk.node->success == TRUE) {
			Walk success_walk = copy_walk(walk, nda.nb_counters);
			push(&success_walks, success_walk);
		}

		if (walk.nb_labels_used == nb_labels) {
			free(walk.counters);
			continue;
		}

		int next_label = labels[walk.nb_labels_used];

		for (int i = 0; i < walk.node->nb_arrows; i ++) {

			Arrow arrow = walk.node->arrows[i];

			// follow the arrow if the labels are equal
			if (arrow.label == next_label) {
				Walk next_walk = copy_walk(walk, nda.nb_counters);
				int success = apply_counters_actions(next_walk, arrow);
				if (success == TRUE) {
					next_walk.nb_labels_used ++;
					next_walk.node = &nda.nodes[arrow.dest];
					push(&walks_to_go, next_walk);
				}
				else free(next_walk.counters);
			}
			// or follow the epsilon-transition
			else if (arrow.label == LABEL_EPSILON) {
				Walk next_walk = copy_walk(walk, nda.nb_counters);
				int success = apply_counters_actions(next_walk, arrow);
				if (success == TRUE) {
					next_walk.node = &nda.nodes[arrow.dest];
					push(&walks_to_go, next_walk);
				}
				else free(next_walk.counters);
			}
		}

		free(walk.counters);
	}

	return success_walks;
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

Pipe create_Pipe () {
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
