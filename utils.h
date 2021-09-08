#if !defined(_UTILS_)
#define _UTILS_

#include "list.h"

enum node_type { TEMP, CHECK, SECURITY, DROP_OFF };

struct area {
	double node; /* time integrated number in the node  */
	double queue; /* time integrated number in the queue */
	double service; /* time integrated number in service */
};

struct node {
	struct passenger *head;
	struct passenger *tail;
	struct passenger in_service;
	struct passenger *head_second;
	struct passenger *tail_second;
	struct area area;
	double opening_time;
	int stream;
	double active_time;
	int open;
	int number;
	int number1;
	int number2;
	enum passenger_type service_type;
	double completion;
	int id; //Server id
	enum node_type type;
};

double min(double a, double c);
enum passenger_type getPassenger();
//double minNode(struct node *nodes, int len, int *id);
double minNode(struct node nodes[4][246], int *id, int *type);
int minQueue(struct node nodes[4][246], int type, int mode,
	     enum passenger_type pass_type);

#endif
