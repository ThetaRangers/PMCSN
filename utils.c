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
	long res = Bernoulli(0.3);

	if (res == 1)
		return FIRST_CLASS;

	return SECOND_CLASS;
}

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

int minQueue(struct node nodes[4][248], int type, int mode, enum passenger_type pass_type){
	int id = 0;
	int i = 1;

	int minimum_array[248];
	int minimum_index = 0;
	
	//consider only open nodes
	while(nodes[type][i].open && i < 248){
		if (!mode || pass_type == SECOND_CLASS) {
			if(nodes[type][i].number < nodes[type][id].number) {
				id = i;
			}
		} else {
			if(nodes[type][i].number1 < nodes[type][id].number1) {
				id = i;
			}
		}
		i++;
	}

	i = 0;
	while(nodes[type][i].open && i < 248){
		if (!mode || pass_type == SECOND_CLASS) {
			if(nodes[type][i].number == nodes[type][id].number) {
				minimum_array[minimum_index] = i;	
				minimum_index++;
			}
		} else {
			if(nodes[type][i].number1 == nodes[type][id].number1) {
				minimum_array[minimum_index] = i;	
				minimum_index++;
			}
		}
		i++;
	}

	SelectStream(246);
	minimum_index = Equilikely(0, minimum_index - 1);

	return minimum_array[minimum_index];
}

