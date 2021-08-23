#include <stdio.h>
#include <stdlib.h>
#include "list.h"

#define handle_error(msg)                                                      \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

//TODO arrivals 
void enqueue(struct passenger **head, struct passenger **tail,
	     enum passenger_type type)
{
	struct passenger *node =
		(struct passenger *)malloc(sizeof(struct passenger));
	if (node == NULL)
		handle_error("malloc");

	node->type = type;
	node->next = NULL;
	/* Append to client list in O(1) time by keeping
	 * reference to last node
	 */
	if (*tail)
		(*tail)->next = node;
	else
		*head = node;

	*tail = node;
}

enum passenger_type dequeue(struct passenger **head, struct passenger **tail)
{
	struct passenger *node = *head;

	if (!node->next)
		*tail = NULL;

	*head = node->next;

	enum passenger_type ret = node->type;
	free(node);
	return ret;
}
