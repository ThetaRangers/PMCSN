#if !defined(_UTILS_)
#define _UTILS_

#include "list.h"

enum node_type { TEMP, CHECK, SECURITY };

struct node {
	struct passenger *head;
	struct passenger *tail;
    int number;
	double completion;
	int id; //Server id
    enum node_type type;
};

double min(double a, double c);
enum passenger_type getPassenger();
double minNode(struct node *nodes, int len, int *id);

#endif
