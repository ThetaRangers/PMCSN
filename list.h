#if !defined( _LIST_ )
#define _LIST_

enum passenger_type {
	FIRST_CLASS,
	SECOND_CLASS
};

struct passenger {
	enum passenger_type type;
	struct passenger* next;
	struct passenger* prev;
};

struct passenger *append_node(enum passenger_type type);
enum passenger_type dequeue_node();
void remove_node(struct passenger *node);
void remove_all();

#endif
