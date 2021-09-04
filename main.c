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

//904914315
//1434868289
#define SEED 123456789

#define START 0.0 /* initial time */
#define STOP 20000.0 /* terminal (close the door) time */
//#define INFINITY (100.0 * STOP) /* must be much larger than STOP  */

#define TEMP_NODE 5
#define CHECK_NODE 20
#define SECURITY_NODE 20
#define DROPOFF_ONLINE 3

#define MAX_SERVERS 248

#define MAX_TEMP 10
#define MAX_CHECK 100
#define MAX_SECURITY 100
#define MAX_DROP_OFF 10

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

#define ARRIVAL_MEAN 0.15
#define TEMP_MEAN 0.2
#define CHECK_MEAN 5
#define CHECK_DROP_MEAN 2
#define SECURITY_MEAN 3
#define DROPOFF_MEAN 2

#define CHECK_PROB 0.7

#define UTIL_THRESHOLD 0.4

#define REPETITIONS 1000
#define STOP_FINITE 360
#define SAMPLE_INTERVAL 1

FILE *st_file; //Service time
FILE *node_population_file; //Node population

double arrival = START;

double income = 0;
double income_24 = 0;
int febbra = 0;
int online = 0;
int dropoff = 0;
int normal = 0;

int current_temp = TEMP_NODE;
int current_checkin = CHECK_NODE;
int current_security = SECURITY_NODE;
int current_dropoff = DROPOFF_ONLINE;

//Mean for different times
double mean_0_6 = 1.2;
double mean_6_18 = 0.15;
double mean_18_24 = 1;

int nodesNumber = TEMP_NODE + CHECK_NODE + SECURITY_NODE + DROPOFF_ONLINE;

struct node servers[4][MAX_SERVERS];

int count_active(int block) {
	int active = 0;
	int i = 0;
	int max_servers;

	switch(block) {
		case 0:
		max_servers = MAX_TEMP;
		break;
		case 1:
		max_servers = MAX_CHECK;
		break;
		case 2:
		max_servers = MAX_SECURITY;
		break;
		case 3:
		max_servers = MAX_DROP_OFF;
		break;
		default:
		max_servers = MAX_SERVERS;
		break;
	}
	
	while (servers[block][i].open && i < max_servers) {
		active++;
		i++;
	}

	return active;
}

void deactivate(int block) {
	int i = 0;
	int max_servers;

	switch(block) {
		case 0:
		max_servers = MAX_TEMP;
		break;
		case 1:
		max_servers = MAX_CHECK;
		break;
		case 2:
		max_servers = MAX_SECURITY;
		break;
		case 3:
		max_servers = MAX_DROP_OFF;
		break;
		default:
		max_servers = MAX_SERVERS;
		break;
	}

	while (servers[block][i].open && i < max_servers)
		i++;
	if (i != 0)
		servers[block][i - 1].open = 0;
}

void activate(int block) {
	int i = 0;
	int max_servers;

	switch(block) {
		case 0:
		max_servers = MAX_TEMP;
		break;
		case 1:
		max_servers = MAX_CHECK;
		break;
		case 2:
		max_servers = MAX_SECURITY;
		break;
		case 3:
		max_servers = MAX_DROP_OFF;
		break;
		default:
		max_servers = MAX_SERVERS;
		break;
	}

	while (servers[block][i].open && i < max_servers)
		i++;
	servers[block][i].open = 1;
}
/*
int getDestination(enum node_type type, int *dest_type)
{
	double rand;
	int active = 0;
	switch (type) {
	case TEMP:
		SelectStream(253);
		active = count_active(0);
		*dest_type = 0;
		return Equilikely(0, active - 1);
	case CHECK:
		SelectStream(252);
		rand = Random();
		if (rand < CHECK_PERC * (1 - FEVER_PERC)) {
			normal++;
			SelectStream(250);
			active = count_active(1);
			*dest_type = 1;

			return Equilikely(0, active - 1);
		} else if (rand <
			   (CHECK_PERC + ONLINE_PERC) * (1 - FEVER_PERC)) {
			SelectStream(249);
			rand = Random();

			if (rand < 0.4) {
				dropoff++;
				SelectStream(248);
				*dest_type = 3;

				active = count_active(3);

				return Equilikely(0, active - 1);
			} else {
				online++;

				*dest_type = 2;
				active = count_active(2);
				
				return Equilikely(0, active - 1);
			}
		} else {
			febbra++;
			*dest_type = -1;
			return -1;
		}
	case SECURITY:
		SelectStream(251);
		*dest_type = 2;
		active = count_active(2);
		return Equilikely(0, active - 1);
	default:
		return 0;
	}
}*/


 int getDestination(enum node_type type, int *dest_type)
{
	double rand;
	switch (type) {
	case TEMP:
		*dest_type = 0;
		return minQueue(servers,0);
	case CHECK:
		SelectStream(252);
		rand = Random();
		if (rand < CHECK_PERC * (1 - FEVER_PERC)) {
			normal++;

			*dest_type = 1;
			return minQueue(servers, 1);
		} else if (rand <
			   (CHECK_PERC + ONLINE_PERC) * (1 - FEVER_PERC)) {
			SelectStream(249);
			rand = Random();

			if (rand < 0.4) {
				dropoff++;

				*dest_type = 3;
				return minQueue(servers, 3);
			} else {
				online++;
				
				*dest_type = 2;
				return minQueue(servers, 2);
			}
		} else {
			febbra++;
			*dest_type = -1;
			return -1;
		}
	case SECURITY:
		*dest_type = 2;
		return minQueue(servers, 2);
	default:
		return 0;
	}
}

double getArrival(double current)
{
	SelectStream(254);
	double hour = ((int) current/60) % 24;

	if(hour < 6) {
		arrival += Exponential(mean_0_6);
	} else if(hour < 18) {
		arrival += Exponential(mean_6_18);
	} else {
		arrival += Exponential(mean_18_24);
	}

	return (arrival);
}

double getArrivalStatic() {
	SelectStream(254);
	arrival += Exponential(ARRIVAL_MEAN);
	
	return arrival;
}

double getService(enum node_type type, int id)
{
	SelectStream(servers[type][id].stream);
	double x;

	switch (type) {
	case TEMP:
		return Exponential(TEMP_MEAN);
	case CHECK:
		//x = Hyperexponential(CHECK_MEAN, CHECK_PROB);
		//printf("AGAGA: %lf\n",x);
		//return x;
		return CheckinDistribution(CHECK_MEAN, CHECK_DROP_MEAN, CHECK_PROB);;
	case SECURITY:
		//Truncated Normal
		return TruncatedNormal(SECURITY_MEAN, 2, 0.5, 5);
		//return Exponential(SECURITY_MEAN);
	case DROP_OFF:
		return Exponential(DROPOFF_MEAN);
	default:
		return 0;
	}
}

int simulate(int mode)
{
	char response_filename[30] = "";
	char node_population_filename[30] = "";

	if (mode == 0) {
		strcpy(response_filename, "response.csv");
		strcpy(node_population_filename, "node_population.csv");
	} else {
		strcpy(response_filename, "response_improved.csv");
		strcpy(node_population_filename,
		       "node_population_improved.csv");
	}

	int fd1 = open(response_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	int fd2 = open(node_population_filename, O_WRONLY | O_CREAT | O_TRUNC,
		       0644);

	if (fd1 == -1 || fd2 == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	st_file = fdopen(fd1, "w");
	node_population_file = fdopen(fd2, "w");

	double batch_position = 0;
	double batches = 0;
	double batch_mean = 0;
	double mean = 0;
	double variance = 0;
	double stdev;
	int tot_completions = 0;

	double batch_position_second_class = 0;
	double batches_second_class = 0;
	double batch_second_class_mean = 0;
	double second_class_mean = 0;
	double second_class_variance = 0;
	double second_class_stdev;

	double batch_position_first_class = 0;
	double batches_first_class = 0;
	double batch_first_class_mean = 0;
	double first_class_mean = 0;
	double first_class_variance = 0;
	double first_class_stdev;

	fprintf(st_file, "TIME, RESPONSE, POPULATION\n");
	fprintf(node_population_file, "TIME, ID, POPULATION\n");

	if (st_file == NULL || node_population_file == NULL) {
		perror("fdopen");
		exit(EXIT_FAILURE);
	}

	//struct node nodes[nodesNumber];

	struct {
		double arrival; /* next arrival time */
		double current; /* current time */
		double next; /* next (most imminent) event time */
		double last; /* last arrival time */
	} t;

	long number = 0; /* Number in the network */

	t.current = START; /* set the clock */
	t.arrival = getArrival(t.current); /* schedule the first arrival */

	//Initialize nodes
	for(int j = 0; j < 4; j++) {
		for (int i = 0; i < MAX_SERVERS; i++) {
			servers[j][i].completion = INFINITY;
			servers[j][i].head = NULL;
			servers[j][i].tail = NULL;
			servers[j][i].head_second = NULL;
			servers[j][i].tail_second = NULL;
			servers[j][i].id = i;
			servers[j][i].number = 0;

			servers[j][i].open = 0;

			servers[j][i].area.node = 0;
			servers[j][i].area.queue = 0;
			servers[j][i].area.service = 0;
		}
	}

	//Set starting servers
	int current_stream = 0;

	for(int i=0; i < MAX_TEMP; i++) {
		if (i < current_temp)
			servers[0][i].open = 1;
		servers[0][i].stream = current_stream;
		current_stream++;
	}
	for(int i = 0; i < MAX_CHECK; i++) {
		if (i < current_checkin)
			servers[1][i].open = 1;
		servers[1][i].stream = current_stream;
		current_stream++;
	}
	for(int i = 0; i < MAX_SECURITY; i++) {
		if (i < current_security)
			servers[2][i].open = 1;
		servers[2][i].stream = current_stream;
		current_stream++;
	}
	for(int i = 0; i < MAX_DROP_OFF; i++) {
		if (i < current_dropoff)
			servers[3][i].open = 1;
		servers[3][i].stream = current_stream;
		current_stream++;
	}

	int first_class_number = 0;
	int second_class_number = 0;
	int tot_arrivals = 0;

	double util_temp = 0;
	double util_check = 0;
	double util_security = 0;
	double util_dropoff = 0;

	int id;
	int type;
	int dest_type;

	while ((t.arrival < STOP)) {
		double minCompletion = minNode(servers, &id, &type);
		
		t.next = min(t.arrival, minCompletion);

		for(int j = 0; j < 4; j++) {
			for (int i = 0; i < nodesNumber; i++) {
				if (servers[j][i].number > 0) { // update integrals
					servers[j][i].area.node +=
						(t.next - t.current) * servers[j][i].number;
					servers[j][i].area.queue += (t.next - t.current) *
							       (servers[j][i].number - 1);
					servers[j][i].area.service += (t.next - t.current);
				}
			}
		}

		t.current = t.next;

		if (t.current == t.arrival) {
			number++;
			tot_arrivals++;

			int destination = getDestination(TEMP, &dest_type);

			enqueue(&servers[dest_type][destination].head,
				&servers[dest_type][destination].tail, getPassenger(),
				t.arrival);
			t.arrival = getArrivalStatic();

			servers[dest_type][destination].number++;

			if (t.arrival > STOP) {
				t.last = t.current;
				t.arrival = INFINITY;
			}

			if (servers[dest_type][destination].number == 1)
				servers[dest_type][destination].completion =
					t.current +
					getService(dest_type,destination);	

		} else {
			double arrival;
			int destination = 0;
			enum passenger_type pass_type;
			//enum node_type completion_type = servers[type][id].type;
			servers[type][id].number--;

			switch (type) {
			case TEMP:
				dequeue(&servers[type][id].head, &servers[type][id].tail,
					&pass_type, &arrival);

				if (servers[type][id].number > 0)
					servers[type][id].completion =
						t.current +
						getService(type, id);
				else
					servers[type][id].completion = INFINITY;

				destination = getDestination(CHECK, &dest_type);
				
				if (destination != -1) {
					servers[dest_type][destination].number++;

					if (mode == 0 ||
					    pass_type == FIRST_CLASS) {
						enqueue(&servers[dest_type][destination].head,
							&servers[dest_type][destination].tail,
							pass_type, arrival);
					} else {
						enqueue(&servers[dest_type][destination]
								 .head_second,
							&servers[dest_type][destination]
								 .tail_second,
							pass_type, arrival);
					}

					if (servers[dest_type][destination].number == 1)
						servers[dest_type][destination].completion =
							t.current +
							getService(
								dest_type,
								destination);
				} else {
					number--;
				}

				break;
			case DROP_OFF:
			case CHECK:
				if (mode == 0 || servers[type][id].head != NULL) {
					dequeue(&servers[type][id].head,
						&servers[type][id].tail, &pass_type,
						&arrival);
				} else {
					dequeue(&servers[type][id].head_second,
						&servers[type][id].tail_second,
						&pass_type, &arrival);
				}

				if (servers[type][id].number > 0)
					servers[type][id].completion =
						t.current +
						getService(type, id);
				else
					servers[type][id].completion = INFINITY;

				destination = getDestination(SECURITY, &dest_type);

				servers[dest_type][destination].number++;
				if (mode == 0 || pass_type == FIRST_CLASS) {
					enqueue(&servers[dest_type][destination].head,
						&servers[dest_type][destination].tail,
						pass_type, arrival);
				} else {
					enqueue(&servers[dest_type][destination].head_second,
						&servers[dest_type][destination].tail_second,
						pass_type, arrival);
				}

				if (servers[dest_type][destination].number == 1)
					servers[dest_type][destination].completion =
						t.current +
						getService(
							dest_type,
							destination);

				break;
			case SECURITY:
				number--;
				if (mode == 0 || servers[type][id].head != NULL) {
					dequeue(&servers[type][id].head,
						&servers[type][id].tail, &pass_type,
						&arrival);
				} else {
					dequeue(&servers[type][id].head_second,
						&servers[type][id].tail_second,
						&pass_type, &arrival);
				}
				SelectStream(255);
				double time_airport =
					Normal(TIME_IN_AIRPORT, VARIANCE);
				double response_time = (t.current - arrival);
				//printf("RESPONSE TIME %lf\n", response_time);
			
				fprintf(st_file, "%lf, %lf, %ld\n", t.current,
					response_time, number);

				//Welford
				batch_position++;
				double diff = response_time - batch_mean;
				batch_mean += diff / batch_position;

				//Wellford for second class
				int spending;
				if (pass_type == SECOND_CLASS) {
					batch_position_second_class++;
					diff = response_time -
					       batch_second_class_mean;
					batch_second_class_mean +=
						diff /
						batch_position_second_class;

					if (batch_position_second_class ==
					    BATCH_SIZE + 1) {
						batches_second_class++;
						diff = batch_second_class_mean -
						       second_class_mean;
						second_class_variance +=
							diff * diff *
							((batches_second_class -
							  1.0) /
							 batches_second_class);
						second_class_mean +=
							diff /
							batches_second_class;

						batch_position_second_class = 0;
						batch_second_class_mean = 0;
					}

					spending = SECOND_CLASS_SPENDING;
					second_class_number++;
				} else {
					batch_position_first_class++;
					diff = response_time -
					       batch_first_class_mean;
					batch_first_class_mean +=
						diff /
						batch_position_first_class;

					if (batch_position_first_class ==
					    BATCH_SIZE + 1) {
						batches_first_class++;
						diff = batch_first_class_mean -
						       first_class_mean;
						first_class_variance +=
							diff * diff *
							((batches_first_class -
							  1.0) /
							 batches_first_class);
						first_class_mean +=
							diff /
							batches_first_class;

						batch_position_first_class = 0;
						batch_first_class_mean = 0;
					}

					spending = FIRST_CLASS_SPENDING;
					first_class_number++;
				}

				if (batch_position == BATCH_SIZE + 1) {
					//Total Mean Welford
					batches++;
					diff = batch_mean - mean;
					variance += diff * diff *
						    ((batches - 1.0) / batches);
					mean += diff / batches;

					batch_position = 0;
					batch_mean = 0;
				}

				tot_completions++;

				double time_left = time_airport - response_time;
				if (time_left > 0) {
					income += (time_left / time_airport) *
						  spending;
					if (t.current > STOP - 1440 &&
					    t.current < STOP) {
						income_24 += (time_left /
							      time_airport) *
							     spending;
					}
				}
				if (servers[type][id].number > 0)
					servers[type][id].completion =
						t.current +
						getService(type, id);
				else
					servers[type][id].completion = INFINITY;

				break;
			}

			fprintf(node_population_file, "%lf, %d, %d\n",
				t.current, id, servers[type][id].number);
		}
	}

	for(int j = 0; j < 4; j++) {
		for (int i = 0; i < MAX_SERVERS; i++) {
			remove_all(&servers[j][i].head);
			remove_all(&servers[j][i].head_second);
		}
	}

	// Last Welford iteration
	batches++;
	double diff = batch_mean - mean;
	variance += diff * diff * (batches - 1) / batches;
	mean += diff / batches;
	variance = variance / batches;
	stdev = sqrt(variance);

	double t_student = idfStudent(batches - 1, 1 - ALFA / 2);
	double endpoint = t_student * stdev / sqrt(batches - 1);

	// Last Welford iteration for first class
	batches_first_class++;
	diff = batch_first_class_mean - first_class_mean;
	first_class_variance +=
		diff * diff *
		((batches_first_class - 1.0) / batches_first_class);
	first_class_mean += diff / batches_first_class;
	first_class_variance = first_class_variance / batches_first_class;
	first_class_stdev = sqrt(first_class_variance);

	double t_student_first_class =
		idfStudent(batches_first_class - 1, 1 - ALFA / 2);
	double first_class_endpoint = t_student_first_class *
				      first_class_stdev /
				      sqrt(batches_first_class - 1);

	// Last Welford iteration for second class
	batches_second_class++;
	diff = batch_second_class_mean - second_class_mean;
	second_class_variance +=
		diff * diff *
		((batches_second_class - 1.0) / batches_second_class);
	second_class_mean += diff / batches_second_class;
	second_class_variance = second_class_variance / batches_second_class;
	second_class_stdev = sqrt(second_class_variance);

	double t_student_second_class =
		idfStudent(batches_second_class - 1, 1 - ALFA / 2);
	double second_class_endpoint = t_student_second_class *
				       second_class_stdev /
				       sqrt(batches_second_class - 1);

	income -= t.current * SERV_COST * (nodesNumber);
	income_24 -= 1440 * SERV_COST * (nodesNumber);

	fclose(st_file);
	fclose(node_population_file);

	if (mode == 0) {
		printf("ORIGINAL\n");
	} else {
		printf("IMPROVED\n");
	}

	printf("PASSENGERS\n");
	printf("Arrivals............... %d\n", tot_arrivals);
	printf("Completions............ %d\n", tot_completions);
	printf("First Class............ %d\n", first_class_number);
	printf("Second Class........... %d\n", second_class_number);
	printf("Fever.................. %d\n", febbra);
	printf("Online................. %d\n", online);
	printf("Dropoff................ %d\n", dropoff);

	printf("\nINCOME:\n");
	printf("Income for %.1f hours. %lf\n", STOP / 60.0, income);
	printf("Income 24 hours........ %lf\n", income_24);

	printf("\nRESPONSE TIME:\n");
	printf("Batch size............. %d\n", BATCH_SIZE);
	printf("Number of batches...... %d\n", (int)batches);
	printf("Mean................... %lf\n", mean);
	printf("Variance............... %lf\n", variance);
	printf("Stdev.................. %lf\n", stdev);
	printf("Confidence Interval: (%lf, %lf)\n", mean - endpoint,
	       mean + endpoint);

	printf("\nRESPONSE TIME FOR FIRST CLASS:\n");
	printf("Mean................... %lf\n", first_class_mean);
	printf("Variance............... %lf\n", first_class_variance);
	printf("Stdev.................. %lf\n", first_class_stdev);
	printf("Confidence Interval: (%lf, %lf)\n",
	       first_class_mean - first_class_endpoint,
	       first_class_mean + first_class_endpoint);

	printf("\nRESPONSE TIME FOR SECOND CLASS:\n");
	printf("Mean................... %lf\n", second_class_mean);
	printf("Variance............... %lf\n", second_class_variance);
	printf("Stdev.................. %lf\n", second_class_stdev);
	printf("Confidence Interval: (%lf, %lf)\n",
	       second_class_mean - second_class_endpoint,
	       second_class_mean + second_class_endpoint);

	
	printf("\nUTILIZATION:\n");
	for(int j = 0; j < 4; j++) {
	for (int i = 0; i < nodesNumber; i++) {
		char type_string[20];

		if (j == 0) {
			strcpy(type_string, "Temperature");
		} else if (j == 1) {
			strcpy(type_string, "Checkin");
		} else if (j == 2) {
			strcpy(type_string, "Security");
		} else if (j == 3) {
			strcpy(type_string, "Dropoff");
		}
		printf("\t%s-%d......utilization: %lf, average population: %lf\n",
		       type_string, i, servers[j][i].area.service / t.current,
		       servers[j][i].area.node / t.current);
		}
	}

	return 0;
}

void finite_horizon(int mode, int rows, int columns,
		    double statistics[rows][columns])
{
	//struct node nodes[nodesNumber];

	struct {
		double arrival; /* next arrival time */
		double current; /* current time */
		double next; /* next (most imminent) event time */
		double last; /* last arrival time */
	} t;

	long number = 0; /* Number in the network */

	arrival = 0;
	t.current = START; /* set the clock */
	t.arrival = getArrival(t.current); /* schedule the first arrival */

	//Initialize nodes
	for(int j = 0; j < 4; j++) {
		for (int i = 0; i < MAX_SERVERS; i++) {
			servers[j][i].completion = INFINITY;
			servers[j][i].head = NULL;
			servers[j][i].tail = NULL;
			servers[j][i].head_second = NULL;
			servers[j][i].tail_second = NULL;
			servers[j][i].id = i;
			servers[j][i].number = 0;

			servers[j][i].open = 0;

			servers[j][i].area.node = 0;
			servers[j][i].area.queue = 0;
			servers[j][i].area.service = 0;
		}
	}

	//Set starting servers
	int current_stream = 0;

	for(int i=0; i < MAX_TEMP; i++) {
		if (i < current_temp)
			servers[0][i].open = 1;
		servers[0][i].stream = current_stream;
		current_stream++;
	}
	for(int i = 0; i < MAX_CHECK; i++) {
		if (i < current_checkin)
			servers[1][i].open = 1;
		servers[1][i].stream = current_stream;
		current_stream++;
	}
	for(int i = 0; i < MAX_SECURITY; i++) {
		if (i < current_security)
			servers[2][i].open = 1;
		servers[2][i].stream = current_stream;
		current_stream++;
	}
	for(int i = 0; i < MAX_DROP_OFF; i++) {
		if (i < current_dropoff)
			servers[3][i].open = 1;
		servers[3][i].stream = current_stream;
		current_stream++;
	}

	int id;
	int type;
	int dest_type;

	double index = 0;

	for (; index <= t.arrival; index += SAMPLE_INTERVAL) {
		for (int j = 0; j < columns; j++) {
			statistics[(int)(index / SAMPLE_INTERVAL)][columns] = 0;
		}
	}

	while ((t.arrival < STOP_FINITE)) {
		double minCompletion = minNode(servers, &id, &type);

		t.next = min(t.arrival, minCompletion);
	for(int j = 0; j < 4; j++) {
		for (int i = 0; i < nodesNumber; i++) {
			if (servers[j][i].number > 0) { // update integrals
				servers[j][i].area.node +=
					(t.next - t.current) * servers[j][i].number;
				servers[j][i].area.queue += (t.next - t.current) *
						       (servers[j][i].number - 1);
				servers[j][i].area.service += (t.next - t.current);
			}
		}
	}
		//Measure
		double utilization_temp = 0;
		double utilization_check = 0;
		double utilization_dropoff = 0;
		double utilization_security = 0;

		double pop_temp = 0;
		double pop_check = 0;
		double pop_dropoff = 0;
		double pop_security = 0;
		/*
		for (int i = 0; i < TEMP_NODE; i++) {
			utilization_temp += nodes[i].area.service / t.current;
			pop_temp += nodes[i].area.node / t.current;
		}
		utilization_temp = utilization_temp / TEMP_NODE;
		pop_temp = pop_temp / TEMP_NODE;

		for (int i = TEMP_NODE; i < TEMP_NODE + CHECK_NODE; i++) {
			utilization_check += nodes[i].area.service / t.current;
			pop_check += nodes[i].area.node / t.current;
		}
		utilization_check = utilization_check / CHECK_NODE;
		pop_check = pop_check / CHECK_NODE;

		for (int i = TEMP_NODE + CHECK_NODE;
		     i < TEMP_NODE + CHECK_NODE + SECURITY_NODE; i++) {
			utilization_security +=
				nodes[i].area.service / t.current;
			pop_security += nodes[i].area.node / t.current;
		}
		utilization_security = utilization_security / SECURITY_NODE;
		pop_security = pop_security / SECURITY_NODE;

		for (int i = TEMP_NODE + CHECK_NODE + SECURITY_NODE;
		     i < TEMP_NODE + CHECK_NODE + SECURITY_NODE + DROP_OFF;
		     i++) {
			utilization_dropoff +=
				nodes[i].area.service / t.current;
			pop_dropoff += nodes[i].area.node / t.current;
		}
		utilization_dropoff = utilization_dropoff / DROP_OFF;
		pop_dropoff = pop_dropoff / DROP_OFF;

		//Update
		for (; index <= t.next; index += SAMPLE_INTERVAL) {
			statistics[(int)index / SAMPLE_INTERVAL][0] =
				utilization_temp;
			statistics[(int)index / SAMPLE_INTERVAL][1] = pop_temp;

			statistics[(int)index / SAMPLE_INTERVAL][2] =
				utilization_check;
			statistics[(int)index / SAMPLE_INTERVAL][3] = pop_check;

			statistics[(int)index / SAMPLE_INTERVAL][4] =
				utilization_security;
			statistics[(int)index / SAMPLE_INTERVAL][5] =
				pop_security;

			statistics[(int)index / SAMPLE_INTERVAL][6] =
				utilization_dropoff;
			statistics[(int)index / SAMPLE_INTERVAL][7] =
				pop_dropoff;
		}
		*/
		t.current = t.next;

		if (t.current == t.arrival) {
			number++;

			int destination = getDestination(TEMP, &dest_type);

			enqueue(&servers[dest_type][destination].head,
				&servers[dest_type][destination].tail, getPassenger(),
				t.arrival);
			t.arrival = getArrival(t.current);

			servers[dest_type][destination].number++;

			if (t.arrival > STOP) {
				t.last = t.current;
				t.arrival = INFINITY;
			}

			if (servers[dest_type][destination].number == 1)
				servers[dest_type][destination].completion =
					t.current +
					getService(servers[dest_type][destination].type,
						   destination);

		} else {
			double arrival;
			int destination = 0;
			enum passenger_type pass_type;
			//enum node_type completion_type = nodes[id].type;
			servers[type][id].number--;

			switch (type) {
			case TEMP:
				dequeue(&servers[type][id].head, &servers[type][id].tail,
					&pass_type, &arrival);

				if (servers[type][id].number > 0)
					servers[type][id].completion =
						t.current +
						getService(type, id);
				else
					servers[type][id].completion = INFINITY;

				destination = getDestination(CHECK, &dest_type);
				if (destination != -1) {
					servers[dest_type][destination].number++;

					if (mode == 0 ||
					    pass_type == FIRST_CLASS) {
						enqueue(&servers[dest_type][destination].head,
							&servers[dest_type][destination].tail,
							pass_type, arrival);
					} else {
						enqueue(&servers[dest_type][destination]
								 .head_second,
							&servers[dest_type][destination]
								 .tail_second,
							pass_type, arrival);
					}
					if (servers[dest_type][destination].number == 1)
						servers[dest_type][destination].completion =
							t.current +
							getService(
								dest_type,
								destination);
				} else {
					number--;
				}

				break;
			case DROP_OFF:
			case CHECK:
				if (mode == 0 || servers[type][id].head != NULL) {
					dequeue(&servers[type][id].head,
						&servers[type][id].tail, &pass_type,
						&arrival);
				} else {
					dequeue(&servers[type][id].head_second,
						&servers[type][id].tail_second,
						&pass_type, &arrival);
				}

				if (servers[type][id].number > 0)
					servers[type][id].completion =
						t.current +
						getService(type, id);
				else
					servers[type][id].completion = INFINITY;

				destination = getDestination(SECURITY, &dest_type);

				servers[dest_type][destination].number++;
				if (mode == 0 || pass_type == FIRST_CLASS) {
					enqueue(&servers[dest_type][destination].head,
						&servers[dest_type][destination].tail,
						pass_type, arrival);
				} else {
					enqueue(&servers[dest_type][destination].head_second,
						&servers[dest_type][destination].tail_second,
						pass_type, arrival);
				}

				if (servers[dest_type][destination].number == 1)
					servers[dest_type][destination].completion =
						t.current +
						getService(
							type,
							destination);

				break;
			case SECURITY:
				number--;

				if (mode == 0 || servers[type][id].head != NULL) {
					dequeue(&servers[type][id].head,
						&servers[type][id].tail, &pass_type,
						&arrival);
				} else {
					dequeue(&servers[type][id].head_second,
						&servers[type][id].tail_second,
						&pass_type, &arrival);
				}

				if (servers[type][id].number > 0)
					servers[type][id].completion =
						t.current +
						getService(type, id);
				else
					servers[type][id].completion = INFINITY;

				break;
			}
		}
	}

	double utilization_temp = 0;
	double utilization_check = 0;
	double utilization_dropoff = 0;
	double utilization_security = 0;

	double pop_temp = 0;
	double pop_check = 0;
	double pop_dropoff = 0;
	double pop_security = 0;

	t.next = STOP_FINITE;

	/*
	for (int i = 0; i < TEMP_NODE; i++) {
		utilization_temp += nodes[i].area.service / t.current;
		pop_temp += nodes[i].area.node / t.current;
	}
	utilization_temp = utilization_temp / TEMP_NODE;
	pop_temp = pop_temp / TEMP_NODE;

	for (int i = TEMP_NODE; i < TEMP_NODE + CHECK_NODE; i++) {
		utilization_check += nodes[i].area.service / t.current;
		pop_check += nodes[i].area.node / t.current;
	}
	utilization_check = utilization_check / CHECK_NODE;
	pop_check = pop_check / CHECK_NODE;

	for (int i = TEMP_NODE + CHECK_NODE;
	     i < TEMP_NODE + CHECK_NODE + SECURITY_NODE; i++) {
		utilization_security += nodes[i].area.service / t.current;
		pop_security += nodes[i].area.node / t.current;
	}
	utilization_security = utilization_security / SECURITY_NODE;
	pop_security = pop_security / SECURITY_NODE;

	for (int i = TEMP_NODE + CHECK_NODE + SECURITY_NODE;
	     i < TEMP_NODE + CHECK_NODE + SECURITY_NODE + DROP_OFF; i++) {
		utilization_dropoff += nodes[i].area.service / t.current;
		pop_dropoff += nodes[i].area.node / t.current;
	}
	utilization_dropoff = utilization_dropoff / DROP_OFF;
	pop_dropoff = pop_dropoff / DROP_OFF;

	//Update
	for (; index <= t.next; index += SAMPLE_INTERVAL) {
		statistics[(int)index / SAMPLE_INTERVAL][0] = utilization_temp;
		statistics[(int)index / SAMPLE_INTERVAL][1] = pop_temp;

		statistics[(int)index / SAMPLE_INTERVAL][2] = utilization_check;
		statistics[(int)index / SAMPLE_INTERVAL][3] = pop_check;

		statistics[(int)index / SAMPLE_INTERVAL][4] =
			utilization_security;
		statistics[(int)index / SAMPLE_INTERVAL][5] = pop_security;

		statistics[(int)index / SAMPLE_INTERVAL][6] =
			utilization_dropoff;
		statistics[(int)index / SAMPLE_INTERVAL][7] = pop_dropoff;
	}*/

	for(int j = 0; j < 4; j++) {
		for (int i = 0; i < MAX_SERVERS; i++) {
			remove_all(&servers[j][i].head);
			remove_all(&servers[j][i].head_second);
		}
	}
}

int repeat_finite_horizon(int mode)
{
	int columns = 16;

	double results[STOP_FINITE / SAMPLE_INTERVAL + 1][columns];
	double statistics[STOP_FINITE / SAMPLE_INTERVAL + 1][8];

	for (int row = 0; row < STOP_FINITE / SAMPLE_INTERVAL + 1; row++) {
		for (int column = 0; column < 16; column++) {
			results[row][column] = 0;
		}
	}

	for (int i = 1; i < REPETITIONS + 1; i++) {
		finite_horizon(mode, STOP_FINITE / SAMPLE_INTERVAL + 1, 8,
			       statistics);

		for (int j = 1; j < STOP_FINITE / SAMPLE_INTERVAL + 1; j++) {
			for (int columns = 0; columns < 8; columns++) {
				double diff = statistics[j][columns] -
					      results[j][columns];
				results[j][8 + columns] +=
					diff * diff * ((i - 1.0) / i);
				results[j][columns] += diff / i;
			}
		}
	}

	for (int j = 1; j < STOP_FINITE / SAMPLE_INTERVAL + 1; j++) {
		for (int columns = 8; columns < 16; columns++) {
			double variance = results[j][columns] / REPETITIONS;
			double stdev = sqrt(variance);

			double t_student =
				idfStudent(REPETITIONS - 1, 1 - ALFA / 2);
			results[j][columns] =
				t_student * stdev / sqrt(REPETITIONS - 1);
		}
	}

	int fd;

	if (mode == 0) {
		fd = open("finite_horizon.csv", O_WRONLY | O_CREAT | O_TRUNC,
			  0644);
	} else {
		fd = open("finite_horizon_improved.csv",
			  O_WRONLY | O_CREAT | O_TRUNC, 0644);
	}

	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	FILE *file = fdopen(fd, "w");

	fprintf(file,
		"sample,utilization_temp_mean,population_temp_mean,utilization_check_mean,"
		"population_check_mean,utilization_security_mean,population_security_mean,"
		"utilization_dropoff_mean,population_dropoff_mean,utilization_temp_endpoint,"
		"population_temp_endpoint,utilization_check_endpoint,population_check_endpoint,"
		"utilization_security_endpoint,population_security_endpoint,"
		"utilization_dropoff_endpoint,population_dropoff_endpoint\n");

	for (int j = 1; j < STOP_FINITE / SAMPLE_INTERVAL + 1; j++) {
		fprintf(file,
			"%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",
			j * SAMPLE_INTERVAL, results[j][0], results[j][1],
			results[j][2], results[j][3], results[j][4],
			results[j][5], results[j][6], results[j][7],
			results[j][8], results[j][9], results[j][10],
			results[j][11], results[j][12], results[j][13],
			results[j][14], results[j][15]);
	}

	fclose(file);

	return 0;
}

int main(int argc, char **argv)
{
	int mode;

	if (argc != 3) {
		printf("Usage: simulation <mode> (o original, i improved) <finite-infinite> (f finite, i infinite)\n");
		return -1;
	}

	PlantSeeds(SEED);

	if (!strcmp(argv[1], "o")) {
		mode = 0;
	} else if (!strcmp(argv[1], "i")) {
		mode = 1;
	} else {
		printf("Usage: simulation <mode> (o original, i improved) <finite-infinite> (f finite, i infinite)\n");
		return -1;
	}

	if (!strcmp(argv[2], "f")) {
		//return repeat_finite_horizon(mode);
	} else if (!strcmp(argv[2], "i")) {
		return simulate(mode);
	} else {
		printf("Usage: simulation <mode> (o original, i improved) <finite-infinite> (f finite, i infinite)\n");
		return -1;
	}
}
