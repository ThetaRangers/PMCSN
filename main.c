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
#define SECURITY_NODE 3
#define DROPOFF_ONLINE 1

#define FEVER_PERC 0.1
#define ONLINE_PERC 0.6
#define CHECK_PERC 0.4

#define FIRST_CLASS_SPENDING 20
#define SECOND_CLASS_SPENDING 5

enum stream { ARRIVAL, SERVICE, ROUTING };

int income = 0;
int febbra = 0;
int online = 0;
int dropoff = 0;
int normal = 0;

int nodesNumber = TEMP_NODE + CHECK_NODE + SECURITY_NODE + DROPOFF_ONLINE;

int getDestination(enum node_type type)
{
	double rand;
	switch (type) {
	case TEMP:
		SelectStream(253);
		return Equilikely(0, TEMP_NODE - 1);
	case CHECK:
		SelectStream(252);
		rand = Random();
		if (rand < CHECK_PERC * (1 - FEVER_PERC)) {
			rand = rand/CHECK_PERC;
			normal++;
		} else if (rand < (CHECK_PERC + ONLINE_PERC) * (1 - FEVER_PERC)) {
			if(rand > 0.8) {
				dropoff++;
				return TEMP_NODE + CHECK_NODE + SECURITY_NODE;
			} else {
				online++;
				return getDestination(SECURITY);
			}
		} else {
			febbra++;
			return -1;
		}

		int destination = (TEMP_NODE + ((long)CHECK_NODE * rand));

		return destination;
	case SECURITY:
		SelectStream(251);
		return Equilikely(TEMP_NODE + CHECK_NODE, TEMP_NODE + CHECK_NODE + SECURITY_NODE - 1);
	default:
		return 0;
	}
}

double getArrival()
{
	static double arrival = START;

	SelectStream(254);
	arrival += Exponential(0.02);
	return (arrival);
}

double getService(enum node_type type, int id)
{
	SelectStream(id);
	switch (type) {
	case TEMP:
		return Exponential(0.5);
	case CHECK:
		return Normal(0.2, 0.1);
	case SECURITY:
		return (Erlang(5, 0.3));
	case DROP_OFF:
		return Normal(1, 0.1);
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
			nodes[i].type = SECURITY;
		else if (i < TEMP_NODE + CHECK_NODE + SECURITY_NODE + DROPOFF_ONLINE)
			nodes[i].type = DROP_OFF;
	}

	int rikky = 0;
	int povery = 0;
	int tot_arrivals = 0;

	int id;
	while ((t.arrival < STOP) || number > 0) {
		double minCompletion = minNode(nodes, nodesNumber, &id);

		t.next = min(t.arrival, minCompletion);

		t.current = t.next;

		if (t.current == t.arrival) {
			number++;
			tot_arrivals++;
			t.arrival = getArrival();

			int destination = getDestination(TEMP);
			nodes[destination].number++;

			enqueue(&nodes[destination].head,
				&nodes[destination].tail, getPassenger());
			if (t.arrival > STOP) {
				t.last = t.current;
				t.arrival = INFINITY;
			}

			if (nodes[destination].number == 1)
				nodes[destination].completion =
					t.current +
					getService(nodes[destination].type, destination);

		} else {
			//Servizietto
			int destination;
			enum passenger_type pass_type;
			enum node_type completion_type =  nodes[id].type;
			nodes[id].number--;

			switch(completion_type) {
			case TEMP:
				pass_type = dequeue(&nodes[id].head, &nodes[id].tail);

				if (nodes[id].number > 0)
					nodes[id].completion =
						t.current + getService(nodes[id].type, id);
				else
					nodes[id].completion = INFINITY;

				//Maybe remove???
				destination = getDestination(CHECK);
				if(destination != -1) {
					nodes[destination].number++;
					enqueue(&nodes[destination].head,&nodes[destination].tail, pass_type);
					if (nodes[destination].number == 1)
						nodes[destination].completion = t.current + getService(nodes[destination].type, destination);
				} else {
					number--;
				}

				break;
			case DROP_OFF:
			case CHECK:
				pass_type = dequeue(&nodes[id].head, &nodes[id].tail);
				
				if (nodes[id].number > 0)
					nodes[id].completion =
						t.current + getService(nodes[id].type, id);
				else
					nodes[id].completion = INFINITY;
				
				destination = getDestination(SECURITY);

				nodes[destination].number++;
				enqueue(&nodes[destination].head,&nodes[destination].tail, pass_type);
				if (nodes[destination].number == 1)
					nodes[destination].completion = t.current + getService(nodes[destination].type, destination);

				break;
			case SECURITY:
				number--;
				pass_type = dequeue(&nodes[id].head, &nodes[id].tail);
				if (pass_type == FIRST_CLASS) {
					income += FIRST_CLASS_SPENDING;
					rikky++;
				} else {
					income += SECOND_CLASS_SPENDING;
					povery++;
				}
				
				if (nodes[id].number > 0)
					nodes[id].completion =
						t.current + getService(nodes[id].type, id);
				else
					nodes[id].completion = INFINITY;

				break;
			}
		}
	}

	printf("Ricchi: %d , Poveri: %d, Tot Arrivi: %d, Sum completion: %d\n", rikky, povery, tot_arrivals, rikky + povery);
	printf("Febbra: %d, Online: %d, Normale: %d, Dropoff: %d\n", febbra, online, normal, dropoff);
	printf("Income: %d\n", income);

	return 0;
}
