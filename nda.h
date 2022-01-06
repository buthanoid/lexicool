typedef struct NDA NDA;
typedef struct Node Node;
typedef struct Arrow Arrow;
typedef struct Counter_Action Counter_Action;

NDA create_NDA (int nb_nodes_capacity, int nb_counters);
Node * add_Node (NDA * nda, int success, int nb_arrows_capacity);
Arrow * add_arrow (Node * node, int label, int dest, int nb_counters_actions);
Counter_Action new_counter_action (int num_counter, int action, int action_param);

enum { LABEL_EPSILON = -1 }; // constant for epsilon label
// constants for counter actions
enum { ACTION_SET, ACTION_ADD, ACTION_AT_LEAST, ACTION_AT_MOST };

struct NDA {
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

	int nb_counters_actions;
	Counter_Action * counters_actions;
};

struct Counter_Action {
	int num_counter;
	int action;
	int action_param;
};
