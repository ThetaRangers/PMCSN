#include <stdio.h>
#include "rngs.h"
#include "rvgs.h"
#include "list.h"
#include "utils.h"

#define SEED 69420

#define START 0.0 /* initial time */
#define STOP 20000.0 /* terminal (close the door) time */
#define INFINITY (100.0 * STOP) /* must be much larger than STOP  */

#define TEMP_NODE 2
#define CHECK_NODE 4
#define DROPOFF_ONLINE 1
#define SECURITY_NODE 3

enum stream { ARRIVAL, SERVICE, ROUTING };

int nodesNumber = TEMP_NODE + CHECK_NODE + SECURITY_NODE;

void findEvent()
{
}

int getDestination(enum node_type type)
{
	switch (type) {
	case TEMP:
		return Equilikely(0, TEMP_NODE - 1);
	case CHECK:
		return Equilikely(TEMP_NODE, CHECK_NODE - 1);
	case SECURITY:
		return Equilikely(CHECK_NODE, SECURITY_NODE - 1);
	default:
		return 0;
	}
}

double getArrival()
{
	static double arrival = START;

	SelectStream(0);
	arrival += Exponential(2.0); //Change this
	return (arrival);
}

double getService(enum node_type type)
{
	switch (type) {
	case TEMP:
		SelectStream(1);
		return (Erlang(5, 0.3)); //Change
	case CHECK:
		SelectStream(1);
		return (Erlang(5, 0.3)); //Change
	case SECURITY:
		SelectStream(1);
		return (Erlang(5, 0.3)); //Change
	default:
		return 0;
	}
}

int main()
{
	struct node nodes[nodesNumber];

	struct {
		double arrival; /* next arrival time */
		double current; /* current time */
		double next; /* next (most imminent) event time */
		double last; /* last arrival time */
	} t;

	SelectStream(0); /* select the default stream */
	PutSeed(SEED);

	long number = 0; /* Number in the queue */

	t.current = START; /* set the clock */
	t.arrival = getArrival(); /* schedule the first arrival */
	//t.completion = INFINITY; /* the first event can't be a completion */

	//Initialize nodes
	for (int i = 0; i < nodesNumber; i++) {
		nodes[i].completion = INFINITY;
		nodes[i].head = NULL;
		nodes[i].tail = NULL;
		nodes[i].id = i;
		nodes[i].number = 0;

		if (i < TEMP_NODE)
			nodes[i].type = TEMP;
		else if (i < TEMP_NODE + CHECK_NODE)
			nodes[i].type = CHECK;
		else if (i < TEMP_NODE + CHECK_NODE + SECURITY_NODE)
			nodes[i].type = SECURITY_NODE;
	}

	int rikky = 0;
	int povery = 0;

	//SINGOLA TEMPURADURA
	// || (number > 0)
	int id;
	while ((t.arrival < STOP) || number > 0) {
		double minCompletion = minNode(nodes, nodesNumber, &id);
		printf("Min completion: %lf id: %d arrival: %lf\n",
		       minCompletion, id, t.arrival);
		t.next = min(t.arrival, minCompletion);
		t.current = t.next;

		if (t.current == t.arrival) {
			number++;
			t.arrival = getArrival();

			int destination = getDestination(TEMP);
			nodes[destination].number++;
			printf("Routing in %d\n", destination);

			enqueue(&nodes[destination].head,
				&nodes[destination].tail, getPassenger());
			if (t.arrival > STOP) {
				t.last = t.current;
				t.arrival = INFINITY;
			}

			//OK?
			if (nodes[destination].number == 1)
				nodes[destination].completion =
					t.current +
					getService(nodes[destination].type);

		} else {
			//Servizietto
			number--;
			nodes[id].number--;
			printf("Deque in %d\n", id);
			if (dequeue(&nodes[id].head, &nodes[id].tail) ==
			    FIRST_CLASS) {
				rikky++;
			} else {
				povery++;
			}

			if (nodes[id].number > 0)
				nodes[id].completion =
					t.current + getService(nodes[id].type);
			else
				nodes[id].completion = INFINITY;
		}
	}

	printf("Ricchi: %d , Poveri: %d\n", rikky, povery);

	return 0;
}
