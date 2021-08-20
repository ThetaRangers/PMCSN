#include <stdio.h>
#include <stdlib.h>
#include "list.h"

#define handle_error(msg)                                                      \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

struct passenger *last = NULL;
struct passenger *passengers = NULL;

struct passenger *append_node(enum passenger_type type)
{
	struct passenger *node = (struct passenger *)malloc(sizeof(struct passenger));
	if (node == NULL)
		handle_error("malloc");

	node->type = type;
	node->next = NULL;
	node->prev = last;
	/* Append to client list in O(1) time by keeping
	 * reference to last node
	 */
	if (last)
		last->next = node;
	else
		passengers = node;

	last = node;

	return node;
}

void remove_node(struct passenger *node)
{
	/* Remove from list in O(1) time by passing
	 * reference to the node that should be removed
	 */
	if (node->next)
		node->next->prev = node->prev;
	else
		last = node->prev;

	if (node->prev)
		node->prev->next = node->next;
	else
		passengers = node->next;

	free(node);
}


enum passenger_type dequeue_node()
{
	struct passenger *node = passengers;

	if (passengers->next)
		passengers->next->prev = NULL;
	else
		last = NULL;

	passengers = node->next;

	enum passenger_type ret = node->type;
	free(node);
	return ret;
}

void remove_all()
{
	while (passengers) {
		struct passenger *tmp = passengers;
		passengers = passengers->next;
		free(tmp);
	}
}
