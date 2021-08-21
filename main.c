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

enum stream { ARRIVAL, SERVICE, ROUTING };

struct tempNode {
	struct passenger *head;
	int id; //Server id
};

double getArrival()
{
	static double arrival = START;

	SelectStream(0);
	arrival += Exponential(
		2.0); //Change this(?) AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPIEDINISINDACAAAAAAAA
	return (arrival);
}

double getService()
{
	SelectStream(1);
	return (Erlang(5, 0.3)); //Change
}

int main()
{
	struct {
		double arrival; /* next arrival time */
		double completion; /* next completion time */
		double current; /* current time */
		double next; /* next (most imminent) event time */
		double last; /* last arrival time */
	} t;

	SelectStream(0); /* select the default stream */
	PutSeed(SEED);

	long number = 0; /* Number in the queue */

	t.current = START; /* set the clock */
	t.arrival = getArrival(); /* schedule the first arrival */
	t.completion = INFINITY; /* the first event can't be a completion */

	struct passenger *tempHead = NULL;
	struct passenger *tempTail = NULL;

	int rikky = 0;
	int povery = 0;

	//SINGOLA TEMPURADURA
	// || (number > 0)
	while ((t.arrival < STOP) || number > 0) {
		t.next = min(t.arrival, t.completion);
		t.current = t.next;

		if (t.current == t.arrival) {
			number++;
			t.arrival = getArrival();
			enqueue(&tempHead, &tempTail, getPassenger());
			if (t.arrival > STOP) {
				t.last = t.current;
				t.arrival = INFINITY;
			}

			if (number == 1)
				t.completion = t.current + getService();
		} else {
			//Servizietto
			number--;
			if (dequeue(&tempHead, &tempTail) == FIRST_CLASS) {
				printf("Ricco\n");
				rikky++;
			} else {
				printf("Povero\n");
				povery++;
			}

			if (number > 0)
				t.completion = t.current + getService();
			else
				t.completion = INFINITY;
		}
	}

	printf("Ricchi: %d , Poveri: %d\n", rikky, povery);

	return 0;
}
