#include <stdio.h>
#include "rngs.h"
#include "rvgs.h"
#include "list.h"
#include "utils.h"

#define SEED 1233456

#define START 0.0 /* initial time */
#define STOP 20000.0 /* terminal (close the door) time */
#define INFINITY (100.0 * STOP) /* must be much larger than STOP  */

#define TEMP_NODE 10
#define CHECK_NODE 20
#define SECURITY_NODE 15
#define DROPOFF_ONLINE 1

#define TIME_IN_AIRPORT 180
#define VARIANCE 20
//Number of minutes in a month 43200 2000euro 0.046
#define SERV_COST 0.046

#define FEVER_PERC 0.1
#define ONLINE_PERC 0.6
#define CHECK_PERC 0.4

#define FIRST_CLASS_SPENDING 50
#define SECOND_CLASS_SPENDING 20

enum stream { ARRIVAL, SERVICE, ROUTING };

double income = 0;
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
	arrival += Exponential(0.2);
	return (arrival);
}

double getService(enum node_type type, int id)
{
	SelectStream(id);
	switch (type) {
	case TEMP:
		return Exponential(0.2);
	case CHECK:
		return Exponential(5);
	case SECURITY:
		//return Hyperexponential(3, 0.99);
		return Exponential(3);
	case DROP_OFF:
		return Exponential(0.5);
	default:
		return 0;
	}
}

int sanPancrazio_aiutaci_tu()
{
	PlantSeeds(SEED);
	SelectStream(251);
	long blabla[SECURITY_NODE] = {0};
	for (int i = 0; i < 10000; i++) {
		long val = Equilikely(TEMP_NODE + CHECK_NODE, TEMP_NODE + CHECK_NODE + SECURITY_NODE - 1);
		blabla[val - (TEMP_NODE + CHECK_NODE)]++;
		printf("%ld\n", val);
	}

	for (int i = 0; i < SECURITY_NODE; i++) {
		printf("index: %d, number: %ld\n", i, blabla[i]);
	}
	return 1;
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

	struct {
		double node; /* time integrated number in the node  */
		double queue;  /* time integrated number in the queue */
		double service;  /* time integrated number in service   */
	} area = {0.0, 0.0, 0.0};

	SelectStream(0); /* select the default stream */
	PlantSeeds(SEED);

	long number = 0; /* Number in the network */

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

		if (number > 0)  { /* update integrals  */
			area.node    += (t.next - t.current) * number;
			area.queue   += (t.next - t.current) * (number - 1);
			area.service += (t.next - t.current);
		}

		t.current = t.next;

		if (t.current == t.arrival) {
			number++;
			tot_arrivals++;
			
			int destination = getDestination(TEMP);
			
			enqueue(&nodes[destination].head,
				&nodes[destination].tail, getPassenger(), t.arrival);
			t.arrival = getArrival();

			
			nodes[destination].number++;

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
			double arrival;
			int destination;
			enum passenger_type pass_type;
			enum node_type completion_type =  nodes[id].type;
			nodes[id].number--;

			switch(completion_type) {
			case TEMP:
				dequeue(&nodes[id].head, &nodes[id].tail, &pass_type, &arrival);

				if (nodes[id].number > 0)
					nodes[id].completion =
						t.current + getService(nodes[id].type, id);
				else
					nodes[id].completion = INFINITY;

				//printf("Temp Service time: %lf In Queue %d: %d\n", t.current - arrival, id, nodes[id].number);

				//Maybe remove???
				destination = getDestination(CHECK);
				if(destination != -1) {
					nodes[destination].number++;
					enqueue(&nodes[destination].head,&nodes[destination].tail, pass_type, arrival);
					if (nodes[destination].number == 1)
						nodes[destination].completion = t.current + getService(nodes[destination].type, destination);
				} else {
					number--;
				}

				break;
			case DROP_OFF:
			case CHECK:
				dequeue(&nodes[id].head, &nodes[id].tail, &pass_type, &arrival);
				
				if (nodes[id].number > 0)
					nodes[id].completion =
						t.current + getService(nodes[id].type, id);
				else
					nodes[id].completion = INFINITY;
				
				printf("Chck Service time: %lf In Queue %d: %d\n", t.current - arrival, id, nodes[id].number);
				
				destination = getDestination(SECURITY);
				//printf("%d\n", destination);

				nodes[destination].number++;
				enqueue(&nodes[destination].head, &nodes[destination].tail, pass_type, arrival);
				if (nodes[destination].number == 1)
					nodes[destination].completion = t.current + getService(nodes[destination].type, destination);

				break;
			case SECURITY:
				number--;
				dequeue(&nodes[id].head, &nodes[id].tail, &pass_type, &arrival);

				double time_airport = Normal(TIME_IN_AIRPORT, VARIANCE);
				int spending;
				if (pass_type == FIRST_CLASS) {
					spending = FIRST_CLASS_SPENDING;
					rikky++;
				} else {
					spending = SECOND_CLASS_SPENDING;
					povery++;
				}

				double time_left = time_airport - (t.current - arrival);
				if(time_left > 0) {
					income += (time_left/time_airport) * spending;
					//printf("This passenger spent %lf\n", (time_left/time_airport) * spending);
				}

				printf("Secu Service time: %lf In Queue %d: %d\n", t.current - arrival, id, nodes[id].number);
				
				if (nodes[id].number > 0)
					nodes[id].completion =
						t.current + getService(nodes[id].type, id);
				else
					nodes[id].completion = INFINITY;

				break;
			}
		}

	}
	
	income -= t.current * SERV_COST * (nodesNumber - TEMP_NODE);

	printf("Ricchi: %d , Poveri: %d, Tot Arrivi: %d, Sum completion: %d\n", rikky, povery, tot_arrivals, rikky + povery);
	printf("Febbra: %d, Online: %d, Normale: %d, Dropoff: %d\n", febbra, online, normal, dropoff);
	printf("Income: %lf\n", income);

	return 0;
}
