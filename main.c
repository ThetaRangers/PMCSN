#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "rngs.h"
#include "rvgs.h"
#include "list.h"
#include "utils.h"
#include "rvms.h"

#define SEED 9876543

#define START 0.0 /* initial time */
#define STOP 20000.0 /* terminal (close the door) time */
//#define INFINITY (100.0 * STOP) /* must be much larger than STOP  */

#define TEMP_NODE 5
#define CHECK_NODE 20
#define SECURITY_NODE 20
#define DROPOFF_ONLINE 3

#define TIME_IN_AIRPORT 180
#define VARIANCE 20
//Number of minutes in a month 2000euro/43200minutes 0.046 euro/minutes
#define SERV_COST 0.046

#define FEVER_PERC 0.1
#define ONLINE_PERC 0.6
#define CHECK_PERC 0.4
#define DROPOFF_PERC 0.4

#define FIRST_CLASS_SPENDING 50
#define SECOND_CLASS_SPENDING 20

#define BATCH_SIZE 512
#define ALFA 0.05

#define ARRIVAL_MEAN 0.2
#define TEMP_MEAN 0.2
#define CHECK_MEAN 5
#define SECURITY_MEAN 3
#define DROPOFF_MEAN 1

FILE *st_file; //Service time
FILE *node_population_file; //Node population

double income = 0;
double income_24 = 0;
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
			//rand = rand/CHECK_PERC;
			normal++;
			SelectStream(250);
			return Equilikely(TEMP_NODE, TEMP_NODE + CHECK_NODE - 1);
		} else if (rand < (CHECK_PERC + ONLINE_PERC) * (1 - FEVER_PERC)) {
			SelectStream(249);
			rand = Random();

			if(rand < 0.4) {
				dropoff++;
				SelectStream(248);
				return Equilikely(TEMP_NODE + CHECK_NODE + SECURITY_NODE, TEMP_NODE + CHECK_NODE + SECURITY_NODE + DROPOFF_ONLINE - 1);
				//return TEMP_NODE + CHECK_NODE + SECURITY_NODE;
			} else {
				online++;
				return getDestination(SECURITY);
			}
		} else {
			febbra++;
			return -1;
		}

		//int destination = (TEMP_NODE + ((long)CHECK_NODE * rand));

		//return destination;
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
	arrival += Exponential(ARRIVAL_MEAN);
	return (arrival);
}

double getService(enum node_type type, int id)
{
	SelectStream(id);
	switch (type) {
	case TEMP:
		return Exponential(TEMP_MEAN);
	case CHECK:
		return Exponential(CHECK_MEAN);
	case SECURITY:
		//return Hyperexponential(3, 0.99);
		return Exponential(SECURITY_MEAN);
	case DROP_OFF:
		return Exponential(DROPOFF_MEAN);
	default:
		return 0;
	}
}

int original()
{	
	int fd1 = open("response.csv", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	int fd2 = open("node_population.csv", O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (fd1 == -1 || fd2 == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	st_file = fdopen(fd1, "w");
	node_population_file = fdopen(fd2, "w");

	double batch_position = 0;
	double batches = 0;
	double batch_mean = 0;
	double batch_variance = 0;
	double mean = 0;
	double variance = 0;
	double stdev;
	int tot_completions = 0;

	fprintf(st_file, "TIME, RESPONSE, POPULATION\n");
	fprintf(node_population_file, "TIME, ID, POPULATION\n");

	if (st_file == NULL || node_population_file == NULL) {
		perror("fdopen");
		exit(EXIT_FAILURE);
	}

	struct node nodes[nodesNumber];

	struct {
		double arrival; /* next arrival time */
		double current; /* current time */
		double next; /* next (most imminent) event time */
		double last; /* last arrival time */
	} t;

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

		nodes[i].area.node = 0;
		nodes[i].area.queue = 0;
		nodes[i].area.service = 0;

		if (i < TEMP_NODE)
			nodes[i].type = TEMP;
		else if (i < TEMP_NODE + CHECK_NODE)
			nodes[i].type = CHECK;
		else if (i < TEMP_NODE + CHECK_NODE + SECURITY_NODE)
			nodes[i].type = SECURITY;
		else if (i < TEMP_NODE + CHECK_NODE + SECURITY_NODE + DROPOFF_ONLINE)
			nodes[i].type = DROP_OFF;
	}

	int first_class_number = 0;
	int second_class_number = 0;
	int tot_arrivals = 0;

	int id;
	while ((t.arrival < STOP) || number > 0) {
		double minCompletion = minNode(nodes, nodesNumber, &id);

		t.next = min(t.arrival, minCompletion);

		for(int i = 0; i < nodesNumber; i++) {
			if (nodes[i].number > 0)  { // update integrals
				nodes[i].area.node += (t.next - t.current) * nodes[i].number;
				nodes[i].area.queue += (t.next - t.current) * (nodes[i].number - 1);
				nodes[i].area.service += (t.next - t.current);
			}
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
			int destination = 0;
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
				
				//printf("Chck Service time: %lf In Queue %d: %d\n", t.current - arrival, id, nodes[id].number);
				
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
				double response_time = (t.current - arrival);

				fprintf(st_file, "%lf, %lf, %ld\n", t.current, response_time, number);

				//Welford
				batch_position++;
				double diff  = response_time - batch_mean;
				batch_variance  += diff * diff * ((batch_position - 1.0) / batch_position);
				batch_mean += diff / batch_position;
				
				if(batch_position == BATCH_SIZE + 1) {
					batch_variance = batch_variance / BATCH_SIZE;
					//printf("Batch Mean %d: %lf\n", batches, batch_mean);
					//printf("Batch Variance %d: %lf\n", batches, batch_variance);

					//Total Mean Welford
					batches++;
					diff  = batch_mean - mean;
					variance  += diff * diff * ((batches - 1.0) / batches);
					mean += diff / batches;

					batch_position = 0;
					batch_mean = 0;
					batch_variance = 0;
				}

				int spending;
				if (pass_type == FIRST_CLASS) {
					spending = FIRST_CLASS_SPENDING;					
					first_class_number++;
				} else {
					spending = SECOND_CLASS_SPENDING;
					second_class_number++;
				}

				tot_completions++;

				double time_left = time_airport - response_time;
				if(time_left > 0) {
					income += (time_left/time_airport) * spending;
					if(t.current > STOP - 1440 && t.current < STOP) {
						income_24 += (time_left/time_airport) * spending;
					}

					//printf("This passenger spent %lf\n", (time_left/time_airport) * spending);
				}

				//printf("Secu Service time: %lf In Queue %d: %d\n", t.current - arrival, id, nodes[id].number);
				
				if (nodes[id].number > 0)
					nodes[id].completion =
						t.current + getService(nodes[id].type, id);
				else
					nodes[id].completion = INFINITY;

				break;
			}

			fprintf(node_population_file, "%lf, %d, %d\n", t.current, id, nodes[id].number);
		}
	}
	
	// Last Welford iteration
	batches++;
	double diff  = batch_mean - mean;
	variance  += diff * diff * (batches - 1) / batches;
	mean += diff / batches;
	variance = variance / batches;
	stdev = sqrt(variance);

	double t_student = idfStudent(batches - 1, 1 - ALFA/2);
	double endpoint = t_student * stdev / sqrt(batches - 1);

	income -= t.current * SERV_COST * (nodesNumber);
	income_24 -= 1440 * SERV_COST * (nodesNumber);

	fclose(st_file);
	fclose(node_population_file);

	printf("ORIGINAL\n");
	printf("PASSENGERS\n");
	printf("Arrivals............... %d\n", tot_arrivals);
	printf("Completions............ %d\n", tot_completions);
	printf("First Class............ %d\n", first_class_number);
	printf("Second Class........... %d\n", second_class_number);
	printf("Fever.................. %d\n", febbra);
	printf("Online................. %d\n", online);
	printf("Dropoff................ %d\n", dropoff);

	printf("\nINCOME:\n");
	printf("Income for %.1f hours. %lf\n", STOP/60.0, income);
	printf("Income 24 hours........ %lf\n", income_24);

	printf("\nRESPONSE TIME:\n");
	printf("Batch size............. %d\n", BATCH_SIZE);
	printf("Number of batches...... %d\n", (int)batches);
	printf("Mean................... %lf\n", mean);
	printf("Variance............... %lf\n", variance);
	printf("Stdev.................. %lf\n", stdev);
	printf("Confidence Interval: (%lf, %lf)\n", mean - endpoint, mean + endpoint);

	printf("\nUTILIZATION:\n");
	for(int i = 0; i < nodesNumber; i++) {
		char type_string[20];

		if(nodes[i].type == TEMP) {
			strcpy(type_string, "Temperature");
		} else if(nodes[i].type == CHECK) {
			strcpy(type_string, "Checkin");
		} else if(nodes[i].type == SECURITY) {
			strcpy(type_string, "Security");
		} else if(nodes[i].type == DROP_OFF) {
			strcpy(type_string, "Dropoff");
		}
		printf("\t%s-%d......%lf\n", type_string, i, nodes[i].area.service/t.current);
	}

	return 0;
}

int improved(){
	int fd1 = open("response_improved.csv", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	int fd2 = open("node_population_improved.csv", O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (fd1 == -1 || fd2 == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	st_file = fdopen(fd1, "w");
	node_population_file = fdopen(fd2, "w");

	double batch_position = 0;
	double batches = 0;
	double batch_mean = 0;
	double batch_variance = 0;
	double mean = 0;
	double variance = 0;
	double stdev;
	int tot_completions = 0;

	fprintf(st_file, "TIME, RESPONSE, POPULATION\n");
	fprintf(node_population_file, "TIME, ID, POPULATION\n");

	if (st_file == NULL || node_population_file == NULL) {
		perror("fdopen");
		exit(EXIT_FAILURE);
	}

	struct node nodes[nodesNumber];

	struct {
		double arrival; /* next arrival time */
		double current; /* current time */
		double next; /* next (most imminent) event time */
		double last; /* last arrival time */
	} t;

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
		nodes[i].head_second = NULL;
		nodes[i].tail_second = NULL;
		nodes[i].id = i;
		nodes[i].number = 0;

		nodes[i].area.node = 0;
		nodes[i].area.queue = 0;
		nodes[i].area.service = 0;

		if (i < TEMP_NODE)
			nodes[i].type = TEMP;
		else if (i < TEMP_NODE + CHECK_NODE)
			nodes[i].type = CHECK;
		else if (i < TEMP_NODE + CHECK_NODE + SECURITY_NODE)
			nodes[i].type = SECURITY;
		else if (i < TEMP_NODE + CHECK_NODE + SECURITY_NODE + DROPOFF_ONLINE)
			nodes[i].type = DROP_OFF;
	}

	int first_class_number = 0;
	int second_class_number = 0;
	int tot_arrivals = 0;

	int id;
	while ((t.arrival < STOP) || number > 0) {
		double minCompletion = minNode(nodes, nodesNumber, &id);

		t.next = min(t.arrival, minCompletion);

		for(int i = 0; i < nodesNumber; i++) {
			if (nodes[i].number > 0)  { // update integrals
				nodes[i].area.node += (t.next - t.current) * nodes[i].number;
				nodes[i].area.queue += (t.next - t.current) * (nodes[i].number - 1);
				nodes[i].area.service += (t.next - t.current);
			}
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
			int destination = 0;
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
				
				//printf("Chck Service time: %lf In Queue %d: %d\n", t.current - arrival, id, nodes[id].number);
				
				destination = getDestination(SECURITY);
				//printf("%d\n", destination);

				nodes[destination].number++;
				if(pass_type == FIRST_CLASS) {
					enqueue(&nodes[destination].head, &nodes[destination].tail, pass_type, arrival);
				} else {
					enqueue(&nodes[destination].head_second, &nodes[destination].tail_second, pass_type, arrival);
				}

				if (nodes[destination].number == 1)
					nodes[destination].completion = t.current + getService(nodes[destination].type, destination);

				break;
			case SECURITY:
				number--;

				if(nodes[id].head != NULL) {
					dequeue(&nodes[id].head, &nodes[id].tail, &pass_type, &arrival);
				} else {
					dequeue(&nodes[id].head_second, &nodes[id].tail_second, &pass_type, &arrival);
				}

				double time_airport = Normal(TIME_IN_AIRPORT, VARIANCE);
				double response_time = (t.current - arrival);

				fprintf(st_file, "%lf, %lf, %ld\n", t.current, response_time, number);

				//Welford
				batch_position++;
				double diff  = response_time - batch_mean;
				batch_variance  += diff * diff * ((batch_position - 1.0) / batch_position);
				batch_mean += diff / batch_position;
				
				if(batch_position == BATCH_SIZE + 1) {
					batch_variance = batch_variance / BATCH_SIZE;
					//printf("Batch Mean %d: %lf\n", batches, batch_mean);
					//printf("Batch Variance %d: %lf\n", batches, batch_variance);

					//Total Mean Welford
					batches++;
					diff  = batch_mean - mean;
					variance  += diff * diff * ((batches - 1.0) / batches);
					mean += diff / batches;

					batch_position = 0;
					batch_mean = 0;
					batch_variance = 0;
				}

				int spending;
				if (pass_type == FIRST_CLASS) {
					spending = FIRST_CLASS_SPENDING;					
					first_class_number++;
				} else {
					spending = SECOND_CLASS_SPENDING;
					second_class_number++;
				}

				tot_completions++;

				double time_left = time_airport - response_time;
				if(time_left > 0) {
					income += (time_left/time_airport) * spending;
					if(t.current > STOP - 1440 && t.current < STOP) {
						income_24 += (time_left/time_airport) * spending;
					}

					//printf("This passenger spent %lf\n", (time_left/time_airport) * spending);
				}

				//printf("Secu Service time: %lf In Queue %d: %d\n", t.current - arrival, id, nodes[id].number);
				
				if (nodes[id].number > 0)
					nodes[id].completion =
						t.current + getService(nodes[id].type, id);
				else
					nodes[id].completion = INFINITY;

				break;
			}

			fprintf(node_population_file, "%lf, %d, %d\n", t.current, id, nodes[id].number);
		}
	}
	
	// Last Welford iteration
	batches++;
	double diff  = batch_mean - mean;
	variance  += diff * diff * (batches - 1) / batches;
	mean += diff / batches;
	variance = variance / batches;
	stdev = sqrt(variance);

	double t_student = idfStudent(batches - 1, 1 - ALFA/2);
	double endpoint = t_student * stdev / sqrt(batches - 1);

	income -= t.current * SERV_COST * (nodesNumber);
	income_24 -= 1440 * SERV_COST * (nodesNumber);

	fclose(st_file);
	fclose(node_population_file);

	printf("IMPROVED\n");
	printf("PASSENGERS\n");
	printf("Arrivals............... %d\n", tot_arrivals);
	printf("Completions............ %d\n", tot_completions);
	printf("First Class............ %d\n", first_class_number);
	printf("Second Class........... %d\n", second_class_number);
	printf("Fever.................. %d\n", febbra);
	printf("Online................. %d\n", online);
	printf("Dropoff................ %d\n", dropoff);

	printf("\nINCOME:\n");
	printf("Income for %.1f hours. %lf\n", STOP/60.0, income);
	printf("Income 24 hours........ %lf\n", income_24);

	printf("\nRESPONSE TIME:\n");
	printf("Batch size............. %d\n", BATCH_SIZE);
	printf("Number of batches...... %d\n", (int)batches);
	printf("Mean................... %lf\n", mean);
	printf("Variance............... %lf\n", variance);
	printf("Stdev.................. %lf\n", stdev);
	printf("Confidence Interval: (%lf, %lf)\n", mean - endpoint, mean + endpoint);

	printf("\nUTILIZATION:\n");
	for(int i = 0; i < nodesNumber; i++) {
		char type_string[20];

		if(nodes[i].type == TEMP) {
			strcpy(type_string, "Temperature");
		} else if(nodes[i].type == CHECK) {
			strcpy(type_string, "Checkin");
		} else if(nodes[i].type == SECURITY) {
			strcpy(type_string, "Security");
		} else if(nodes[i].type == DROP_OFF) {
			strcpy(type_string, "Dropoff");
		}
		printf("\t%s-%d......%lf\n", type_string, i, nodes[i].area.service/t.current);
	}

	return 0;
}


int main(int argc, char **argv){

	if(argc != 2) {
		printf("Usage: simulation <mode> (0 original, 1 improved)\n");
		return 1;
	}

	if(!strcmp(argv[1], "0")) {
		return original();
	} else {
		return improved();
	}
}
