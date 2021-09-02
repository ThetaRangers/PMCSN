#include "utils.h"
#include "rngs.h"
#include "rvgs.h"
#include <stdio.h>

double min(double a, double c)
{
	if (a < c)
		return (a);
	else
		return (c);
}

enum passenger_type getPassenger()
{
	long res = Bernoulli(0.3);

	if (res == 1)
		return FIRST_CLASS;

	return SECOND_CLASS;
}

/*
double minNode(struct node *nodes, int len, int *id)
{
	double minCompletion = nodes[0].completion;
	*id = 0;
	for (int i = 1; i < len; i++) {
		minCompletion = min(minCompletion, nodes[i].completion);
		if (nodes[i].completion == minCompletion) {
			*id = i;
		}
	}

	return minCompletion;
}*/

double minNode(struct node nodes[4][248], int *id, int *type)
{
	double minCompletion = nodes[0][0].completion;
	*id = 0;
	*type = 0;

	for(int j = 0; j < 4; j++) {
		int i = 0;
		while (nodes[j][i].open) {
			minCompletion = min(minCompletion, nodes[j][i].completion);
			if (nodes[j][i].completion == minCompletion) {
				*id = i;
				*type = j;
			}
			i++;
		}
	}

	return minCompletion;
}
