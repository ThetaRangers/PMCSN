#include <stdio.h>
#include <stdlib.h>
#include "list.h"

#define handle_error(msg)                                                      \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

void enqueue(struct passenger **head, struct passenger **tail,
	     enum passenger_type type, double arrival, int shengen)
{
	struct passenger *node =
		(struct passenger *)malloc(sizeof(struct passenger));
	if (node == NULL)
		handle_error("malloc");

	node->type = type;
	node->arrival = arrival;
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

void dequeue(struct passenger **head, struct passenger **tail,
	     enum passenger_type *type, double *arrival, int *shengen)
{
	struct passenger *node = *head;

	if (!node->next)
		*tail = NULL;

	*head = node->next;

	*arrival = node->arrival;
	*type = node->type;
	*shengen = node -> shengen;
	
	free(node);
}

void remove_all(struct passenger **head)
{
	struct passenger *node = *head;
	while (node != NULL) {
		struct passenger *tmp = node;
		node = node->next;
		free(tmp);
	}
}
