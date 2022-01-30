#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "automata.h"

/*int main (int argc, char ** argv) {

	Automata automata;
	new_automata(&automata, 1, 1);

	int node0 = add_node(&automata, TRUE, 2);
	int arrow0 = add_label_arrow(&automata, node0, 0, 0, 2);
	add_counter_action(&automata, node0, arrow0, 0, ACTION_ADD, 1);
	add_counter_action(&automata, node0, arrow0, 0, ACTION_AT_MOST, 30);
	add_interval_arrow(&automata, node0, 0, 1, 0, 0);

	char * string_automata = automata_to_string(automata);
	printf("%s\n", string_automata);
	free(string_automata);

	int labels[100] = { 0 };
	int nb_labels = 25;

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

// nb_nodes must be > 0
int new_automata (Automata * res_automata, int nb_nodes_capacity, int nb_counters) {

	Node * nodes = malloc (nb_nodes_capacity * sizeof(Node));
	// no automata created if malloc problem
	if (nodes == NULL && nb_nodes_capacity > 0) { return FALSE; }

	Automata automata = { 0, nb_nodes_capacity, nodes, nb_counters };
	*res_automata = automata;

	return TRUE;
}

// num must be >= 0
// success must be boolean
// nb_arrows must be >= 0
int add_node (Automata * automata, int success, int nb_arrows_capacity) {

	int success_adapt = adapt_capacity(
		automata->nb_nodes + 1, &automata->nb_nodes_capacity,
		sizeof(Node), (void **) &automata->nodes);

	if (success_adapt) {

		Arrow * arrows = malloc (nb_arrows_capacity * sizeof(Arrow));
		// no node added if malloc problem
		if (arrows == NULL && nb_arrows_capacity > 0) { return BAD_NUM; }

		int num_node = automata->nb_nodes ++;

		Node node = { num_node, success, 0, nb_arrows_capacity, arrows };

		automata->nodes[num_node] = node;

		return num_node;
	}
	else return BAD_NUM;
}

int add_arrow (
	Automata * automata, int num_node, int label, int label_max, int epsilon, int dest,
	int nb_counters_actions_capacity
) {

	Node * node = &automata->nodes[num_node];

	int success_adapt = adapt_capacity(
		node->nb_arrows + 1, &node->nb_arrows_capacity, sizeof(Arrow), (void **) &node->arrows);

	if (success_adapt) {

		Counter_Action * counters_actions =
			malloc (nb_counters_actions_capacity * sizeof(Counter_Action));
		// no arrow added if malloc problem
		if (counters_actions == NULL && nb_counters_actions_capacity > 0) { return BAD_NUM; }

		int num_arrow = node->nb_arrows ++;

		Arrow arrow = {
			label, label_max, epsilon, dest, 0, nb_counters_actions_capacity, counters_actions };

		node->arrows[num_arrow] = arrow;

		return num_arrow;
	}
	else return BAD_NUM;
}

int add_label_arrow (
	Automata * automata, int num_node, int label, int dest, int nb_counters_actions_capacity
) {
	return add_arrow(automata, num_node, label, label, FALSE, dest, nb_counters_actions_capacity);
}

int add_epsilon_arrow (
	Automata * automata, int num_node, int dest, int nb_counters_actions_capacity
) {
	return add_arrow(automata, num_node, 0, 0, TRUE, dest, nb_counters_actions_capacity);
}

int add_interval_arrow (
	Automata * automata, int num_node, int label, int label_max,
	int dest, int nb_counters_actions_capacity
) {
	return add_arrow(
		automata, num_node, label, label_max, FALSE, dest, nb_counters_actions_capacity);
}

int add_counter_action (
	Automata * automata, int num_node, int num_arrow, int num_counter, int action, int action_param
) {
	Node node = automata->nodes[num_node];
	Arrow * arrow = &node.arrows[num_arrow];

	int success_adapt = adapt_capacity(
		arrow->nb_counters_actions + 1, &arrow->nb_counters_actions_capacity,
		sizeof(Counter_Action), (void **) &arrow->counters_actions);

	if (success_adapt) {
		int num_counter_action = arrow->nb_counters_actions ++;

	    Counter_Action counter_action = { num_counter, action, action_param };
		arrow->counters_actions[num_counter_action] = counter_action;

		return num_counter_action;
	}
	else return BAD_NUM;
}

int apply_counters_actions (int * counters, Arrow arrow) {
	int success = TRUE;

	for (int i = 0; i < arrow.nb_counters_actions; i ++) {

		Counter_Action counter_action = arrow.counters_actions[i];
		int num_counter = counter_action.num_counter;
		int param = counter_action.action_param;

		switch (counter_action.action) {
			case ACTION_SET:
				counters[num_counter] = param;
				break;
			case ACTION_ADD:
				counters[num_counter] += param;
				break;
			case ACTION_AT_LEAST:
				if (counters[num_counter] < param) {
					success = FALSE;
					i = arrow.nb_counters_actions; // end loop
				}
				break;
			case ACTION_AT_MOST:
				if (counters[num_counter] > param) {
					success = FALSE;
					i = arrow.nb_counters_actions; // end loop
				}
				break;
		}
	}

	return success;
}

int push_point (Exploration * expl, int num_node, int num_arrow_next, int nb_labels_used) {

	int success_adapt_1 = adapt_capacity(
		expl->nb_points + 1, &expl->nb_points_capacity, sizeof(Point), (void **) &expl->points);

	int success_adapt_2 = success_adapt_1 && adapt_capacity(
		expl->nb_counters + expl->nb_counters_by_point, &expl->nb_counters_capacity,
		sizeof(int), (void **) &expl->counters);

	if (success_adapt_1 && success_adapt_2) {

		int num_point = expl->nb_points ++;
		Point new_point = { num_node, num_arrow_next, nb_labels_used };
		expl->points[num_point] = new_point;

		// settings counters to zeroes
		for (int i = 0; i < expl->nb_counters_by_point; i ++) {
			expl->counters[expl->nb_counters] = 0;
			expl->nb_counters ++;
		}

		return num_point;
	}
	else return BAD_NUM;
}

void pop_point (Exploration * expl) {
	expl->nb_points --;
	expl->nb_counters -= expl->nb_counters_by_point;
}

void explore_step (Exploration * expl, Automata automata, int * labels, int nb_labels) {

	// get the top point of the stack
	int num_point = expl->nb_points - 1;
	Point point = expl->points[num_point];
	Node node = automata.nodes[point.num_node];

	// if num_arrow_next does not exist, pop and return
	if (point.num_arrow_next == node.nb_arrows) {
		pop_point(expl);
		return;
	}

	// if num_arrow_next is not the last arrow to explore in the node
	// we store a future point of exploration, with num_arrow_next + 1
	if (point.num_arrow_next < node.nb_arrows - 1) {

		// duplicate the point
		int num_new_point = push_point(
			expl, point.num_node, point.num_arrow_next, point.nb_labels_used);

		// duplicate the counters
		int start_old_counters = num_point * expl->nb_counters_by_point;
		int start_new_counters = num_new_point * expl->nb_counters_by_point;
		for (int i = 0; i < expl->nb_counters_by_point; i ++) {
			expl->counters[start_new_counters + i] = expl->counters[start_old_counters + i];
		}

		// increment arrow of the old point
		expl->points[num_point].num_arrow_next ++;

		// set the new point as current (we always work on the top point)
		num_point = num_new_point;
	}

	// now we try to follow num_arrow_next

	Arrow arrow = node.arrows[point.num_arrow_next];

	int has_next_label = (point.nb_labels_used < nb_labels);
	int next_label;
	if (has_next_label) next_label = labels[point.nb_labels_used];

	// either the next label is the same of the arrow, either the arrow is epsilon
	if (                 // we handle interval arrows and label arrows in the same way
		(has_next_label && next_label >= arrow.label && next_label <= arrow.label_max)
		|| (arrow.epsilon)) {

		int * counters = &expl->counters[num_point * expl->nb_counters_by_point];

		int success_actions = apply_counters_actions(counters, arrow);
		if (success_actions) {

			// move point to the next node
			expl->points[num_point].num_node = arrow.dest;
			expl->points[num_point].num_arrow_next = 0;
			if (! arrow.epsilon) expl->points[num_point].nb_labels_used ++;
		}
		else pop_point(expl);
	}
	else pop_point(expl);

}

void explore_farthest_success_node (
	Automata automata, int start_num_node, int * labels, int nb_labels,
	int * res_num_node, int * res_nb_labels_used, int * res_counters,
	int * res_nb_explorations_steps, int * res_max_nb_points_reached
) {
	// init result values
	*res_num_node = BAD_NUM;
	*res_nb_labels_used = 0;
	for (int i = 0; i < automata.nb_counters; i ++) res_counters[i] = 0;
	*res_nb_explorations_steps = 0;
	*res_max_nb_points_reached = 0;

	Exploration expl = { 0, 0, NULL, automata.nb_counters, 0, 0, NULL };

	push_point(&expl, start_num_node, 0, 0);

	while (expl.nb_points > 0) {
		int num_point = expl.nb_points - 1;
		Point point = expl.points[num_point];

		if (automata.nodes[point.num_node].success) {
			if (point.nb_labels_used > *res_nb_labels_used || *res_num_node == BAD_NUM) {

				*res_num_node = point.num_node;
				*res_nb_labels_used = point.nb_labels_used;

				int start_counter = num_point * expl.nb_counters_by_point;
				for (int i = 0; i < automata.nb_counters; i ++) {
					res_counters[i] = expl.counters[start_counter + i];
				}
			}
		}

		*res_max_nb_points_reached = max_int(*res_max_nb_points_reached, expl.nb_points);

		explore_step(&expl, automata, labels, nb_labels);
		(*res_nb_explorations_steps) ++;
	}

	free_exploration(expl);
}

void free_exploration (Exploration expl) {
	free(expl.points);
	free(expl.counters);
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
				if (arrow.label == arrow.label_max) { sprintf(buffer, "--%i-->", arrow.label); }
				else { sprintf(buffer, "--[%i-%i]-->", arrow.label, arrow.label_max); }
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

	return builder.chars;
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
				if (arrow.label == arrow.label_max) { sprintf(buffer, "%i", arrow.label); }
				else { sprintf(buffer, "[%i-%i]", arrow.label, arrow.label_max); }
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

	return builder.chars;
}
