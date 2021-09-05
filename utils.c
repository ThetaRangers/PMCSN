#include "utils.h"
#include "rngs.h"
#include "rvgs.h"
#include <math.h>
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
	long res = Bernoulli(0.);

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
		for (int i = 0; i < 248; i++) {
			minCompletion = min(minCompletion, nodes[j][i].completion);
			if (nodes[j][i].completion == minCompletion) {
				*id = i;
				*type = j;
			}
		}
	}

	return minCompletion;
}


int minQueue(struct node nodes[4][248], int type){
	int id = 0;
	int i = 0;

	int minimum_array[248];
	int minimum_index = 0;
	
	//consider only open nodes
	while(nodes[type][i].open && i < 248){
		if(nodes[type][i].number < nodes[type][id].number) {
			id = i;
		}

		i++;
	}

	i = 0;
	while(nodes[type][i].open && i < 248){
		if(nodes[type][i].number == nodes[type][id].number) {
			minimum_array[minimum_index] = i;	
			minimum_index++;
		}
		i++;
	}

	//TODO VALID
	SelectStream(246);
	minimum_index = Equilikely(0, minimum_index - 1);

	return minimum_array[minimum_index];
}
