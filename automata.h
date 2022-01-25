// constants for counter actions
enum { ACTION_SET, ACTION_ADD, ACTION_AT_LEAST, ACTION_AT_MOST };

typedef struct Automata Automata;
typedef struct Node Node;
typedef struct Arrow Arrow;
typedef struct Counter_Action Counter_Action;

typedef struct Walk Walk;

typedef struct Stack Stack;

Automata new_automata (int nb_nodes_capacity, int nb_counters);
int add_node (Automata * automata, int success, int nb_arrows_capacity);

int add_label_arrow (
	Automata * automata, int num_node, int label, int dest, int nb_counters_actions_capacity);

int add_epsilon_arrow (
	Automata * automata, int num_node, int dest, int nb_counters_actions_capacity);

int add_counter_action (
	Automata * automata, int num_node, int num_arrow,
	int num_counter, int action, int action_param);

char * automata_to_string (Automata automata);
char * automata_to_dot (Automata automata);
void free_automata (Automata nda);

Stack all_success_walks (Automata nda, int start_node_num, int nb_labels, int * labels);
void free_walk (Walk walk);

void longest_success_walk (
	Automata automata, int start_node_num, int nb_labels, int * labels,
	int * nb_labels_used, int * end_node_num);

void push (Walk walk, Stack * ref_stack);
Walk pop (Stack * ref_stack);

struct Automata {
	int nb_nodes; // number of nodes of the nda
	int nb_nodes_capacity; // capacity of the array storing the nodes
	Node * nodes;

	int nb_counters;
};

struct Node {
	int num;
	int success; // boolean

	int nb_arrows; // number of arrows of the node
	int nb_arrows_capacity; // capacity of the arrays storing the arrows

	Arrow * arrows;
};

struct Arrow {
	int label; // which letter must be read to follow the arrow
	int dest; // the node that is pointed by the arrow. index in the nodes array
	int epsilon; // boolean

	int nb_counters_actions;
	int nb_counters_actions_capacity;
	Counter_Action * counters_actions;
};

struct Counter_Action {
	int num_counter;
	int action;
	int action_param;
};

struct Walk {
	int nb_labels_used;
	int node_num;
	int * counters;
};

struct Stack {
	int nb_walks;
	int nb_walks_capacity;
	Walk * walks;
};
