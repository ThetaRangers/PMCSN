#if !defined(_LIST_)
#define _LIST_

enum passenger_type { FIRST_CLASS, SECOND_CLASS };

struct passenger {
	enum passenger_type type;
	int shengen;
	double arrival;
	struct passenger *next;
};

void enqueue(struct passenger **head, struct passenger **tail,
	     enum passenger_type type, double arrival, int shengen);
void dequeue(struct passenger **head, struct passenger **tail,
	     enum passenger_type *type, double *arrival, int *shengen);
void remove_all(struct passenger **head);

#endif
