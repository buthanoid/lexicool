// constants for counter actions
enum { ACTION_SET, ACTION_ADD, ACTION_AT_LEAST, ACTION_AT_MOST };

typedef struct Automata Automata;
typedef struct Node Node;
typedef struct Arrow Arrow;
typedef struct Counter_Action Counter_Action;

typedef struct Point Point;
typedef struct Exploration Exploration;

int new_automata (Automata * res_automata, int nb_nodes_capacity, int nb_counters);
int add_node (Automata * automata, int success, int nb_arrows_capacity);

int add_label_arrow (
	Automata * automata, int num_node, int label, int dest, int nb_counters_actions_capacity);

int add_epsilon_arrow (
	Automata * automata, int num_node, int dest, int nb_counters_actions_capacity);

int add_interval_arrow (
	Automata * automata, int num_node, int label, int label_max,
	int dest, int nb_counters_actions_capacity
);

int add_counter_action (
	Automata * automata, int num_node, int num_arrow,
	int num_counter, int action, int action_param);

char * automata_to_string (Automata automata);
char * automata_to_dot (Automata automata);
void free_automata (Automata nda);

int apply_counters_actions (int * counters, Arrow arrow);

void explore_step (Exploration * expl, Automata automata, int * labels, int nb_labels);

void explore_farthest_success_node (
	Automata automata, int start_num_node, int * labels, int nb_labels,
	int * res_num_node, int * res_nb_labels_used, int * res_counters,
	int * res_nb_explorations_steps, int * res_max_nb_points_reached
);

void free_exploration (Exploration expl);

struct Automata {
	int nb_nodes; // number of nodes of the nda
	int nb_nodes_capacity; // capacity of the array storing the nodes
	Node * nodes;

	int nb_counters;
};

struct Node {
	int success; // boolean

	int nb_arrows; // number of arrows of the node
	int nb_arrows_capacity; // capacity of the arrays storing the arrows
	Arrow * arrows;
};

struct Arrow {
	int label; // which letter must be read to follow the arrow
	int label_max; // for labels interval: [label-label_max]. disabled by label_max == label
	int epsilon; // boolean TRUE for epsilon arrows, FALSE otherwise
	int dest; // the node that is pointed by the arrow. index in the nodes array

	int nb_counters_actions;
	int nb_counters_actions_capacity;
	Counter_Action * counters_actions;
};

struct Counter_Action {
	int num_counter;
	int action;
	int action_param;
};

struct Point {
	int num_node; // in which node is the point
	int num_arrow_next; // the next arrow to be followed (must be a existing arrow num)
	int nb_labels_used; // the nb of labels used to reach this point
};

struct Exploration {
	int nb_points;
	int nb_points_capacity;
	Point * points;

	int nb_counters_by_point;
	int nb_counters;
	int nb_counters_capacity;
	int * counters; // the counters for each point
};
