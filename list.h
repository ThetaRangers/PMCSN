#if !defined( _LIST_ )
#define _LIST_

enum passenger_type {
	FIRST_CLASS,
	SECOND_CLASS
};

struct passenger {
	enum passenger_type type;
	struct passenger* next;
};

void enqueue(struct passenger **head, struct passenger **tail,enum passenger_type type);
enum passenger_type dequeue(struct passenger **head, struct passenger **tail);

#endif
