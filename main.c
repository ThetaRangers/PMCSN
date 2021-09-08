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
//123456789
#define SEED 1434868289

#define START 0.0 /* initial time */
#define STOP 10000.0 /* terminal (close the door) time */

#define TEMP_NODE 10
#define CHECK_NODE 20
#define SECURITY_NODE 23
#define DROPOFF_ONLINE 10
#define MAX_SERVERS 246

#define MAX_TEMP 10
#define MAX_CHECK 30
#define MAX_SECURITY 25
#define MAX_DROP_OFF 10

#define TIME_IN_AIRPORT 180
#define VARIANCE 20

//Number of minutes in a month 2000euro/43200minutes 0.046 euro/minutes
#define SERV_COST 0.09

#define FEVER_PERC 0.01
#define ONLINE_PERC 0.6
#define CHECK_PERC 0.4
#define DROPOFF_PERC 0.4

#define FIRST_CLASS_SPENDING 20
#define SECOND_CLASS_SPENDING 5

#define BATCH_SIZE 1024
#define ALFA 0.05

#define ARRIVAL_MEAN 0.15

#define TEMP_MEAN 0.2
#define CHECK_MEAN 5
#define CHECK_DROP_MEAN 2
#define SECURITY_MEAN 3
#define DROPOFF_MEAN 2

#define CHECK_PROB 0.7

#define UTIL_THRESHOLD 0.4

#define STOP_FINITE 1440
#define SAMPLE_INTERVAL 1

#define N 65536

int finite_temp_node[3] = { 10, 10, 10 };
int finite_check_node[3] = { 25, 20, 25 };
int finite_security_node[3] = { 15, 25, 20 };
int finite_dropoff_node[3] = { 10, 10, 10 };

FILE *st_file; //Service time
FILE *node_population_file; //Node population

double arr = START;

double cost = 0;
double income = 0;
double income_24 = 0;
int febbra = 0;
int online = 0;
int dropoff = 0;
int normal = 0;

//Mean for different times
double arrival_mean = ARRIVAL_MEAN;

double mean_0_6 = 0.8;
double mean_6_18 = 0.15;
double mean_18_24 = 0.3;

struct node servers[4][MAX_SERVERS];

int count_active(int block)
{
	int active = 0;
	int i = 0;
	int max_servers;

	switch (block) {
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

void deactivate(int block, int count, double current_time)
{
	int i = 0;
	int max_servers;

	switch (block) {
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

	//deactivate last count servers
	for (int j = 0; j < count && i - j > 0; j++) {
		if (servers[block][i - j - 1].open) {
			servers[block][i - j - 1].active_time +=
				current_time -
				servers[block][i - j - 1].opening_time;
		}

		servers[block][i - j - 1].open = 0;
	}
}

void deactivate_all(int block, double current_time)
{
	int count = count_active(block);
	deactivate(block, count, current_time);
}

void activate(int block, int count, double current_time)
{
	int i = 0;
	int max_servers;

	switch (block) {
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
	//activate last count servers
	for (int j = 0; j < count && i + j < max_servers; j++) {
		servers[block][i + j].open = 1;
		servers[block][i + j].opening_time = current_time;
	}
}

void change_servers(int block, int count, double current_time)
{
	deactivate_all(block, current_time);
	activate(block, count, current_time);
}

int getDestination(enum node_type type, int *dest_type, int mode,
		   enum passenger_type pass_type)
{
	double rand;
	switch (type) {
	case TEMP:
		SelectStream(253);
		*dest_type = 0;
		if (mode == 2 || mode == 3)
			return minQueue(servers, 0, 0, pass_type);
		else
			return Equilikely(0, count_active(0) - 1);
	case CHECK:
		SelectStream(252);
		rand = Random();
		if (rand < CHECK_PERC * (1 - FEVER_PERC)) {
			normal++;
			SelectStream(250);
			*dest_type = 1;
			if (mode == 2 || mode == 3)
				return minQueue(servers, 1, mode, pass_type);
			else
				return Equilikely(0, count_active(1) - 1);
		} else if (rand <
			   (CHECK_PERC + ONLINE_PERC) * (1 - FEVER_PERC)) {
			SelectStream(249);
			rand = Random();

			if (rand < 0.4) {
				dropoff++;
				SelectStream(248);
				*dest_type = 3;
				if (mode == 2 || mode == 3)
					return minQueue(servers, 3, mode,
							pass_type);
				else
					return Equilikely(0,
							  count_active(3) - 1);
			} else {
				online++;
				SelectStream(247);
				*dest_type = 2;
				if (mode == 2 || mode == 3)
					return minQueue(servers, 2, mode,
							pass_type);
				else
					return Equilikely(0,
							  count_active(2) - 1);
			}
		} else {
			febbra++;
			*dest_type = -1;
			return -1;
		}
	case SECURITY:
		SelectStream(251);
		*dest_type = 2;
		if (mode == 2 || mode == 3)
			return minQueue(servers, 2, mode, pass_type);
		else
			return Equilikely(0, count_active(2) - 1);
	default:
		return 0;
	}
}

double getArrival(double current)
{
	SelectStream(254);
	double hour = ((int)current / 60) % 24;

	if (hour < 6) {
		arr += Exponential(mean_0_6);
	} else if (hour < 18) {
		arr += Exponential(mean_6_18);
	} else {
		arr += Exponential(mean_18_24);
	}

	return (arr);
}

double getArrivalStatic()
{
	SelectStream(254);
	arr += Exponential(arrival_mean);

	return arr;
}

double getService(enum node_type type, int id)
{
	SelectStream(servers[type][id].stream);
	double x;

	switch (type) {
	case TEMP:
		//return Exponential(TEMP_MEAN);
		return TruncatedExponential(TEMP_MEAN, 0.7, 3);
	case CHECK:
		//x = Hyperexponential(CHECK_MEAN, CHECK_PROB);
		//printf("AGAGA: %lf\n",x);
		//return x;
		x = TruncatedExponential(CHECK_MEAN, 1, 20);
		//x = Exponential(TEMP_MEAN);
		//x = CheckinDistribution(CHECK_MEAN, CHECK_DROP_MEAN, DROPOFF_PERC);
		return x;
	case SECURITY:
		//Truncated Normal
		return TruncatedNormal(SECURITY_MEAN, 2, 1.5, 5);
		//return Exponential(SECURITY_MEAN);
	case DROP_OFF:
		//return Exponential(DROPOFF_MEAN);
		return TruncatedExponential(DROPOFF_MEAN, 1, 10);
	default:
		return 0;
	}
}

int simulate(int statistics, int mode, int n0, int n1, int n2, int n3,
	     double *inc, double ets[2], double ets1[2], double ets2[2])
{
	int nodesNumber = n0 + n1 + n2 + n3;
	cost = 0;
	income = 0;
	arr = 0;

	int fd3;
	FILE *samples = NULL;

	if (statistics) {
		fd3 = open("infinite_samples.csv", O_WRONLY | O_CREAT | O_TRUNC,
			   0644);
		samples = fdopen(fd3, "w");
		if (fd3 == -1) {
			perror("open");
			exit(EXIT_FAILURE);
		}
	}

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

	struct {
		double arrival; /* next arrival time */
		double current; /* current time */
		double next; /* next (most imminent) event time */
		double last; /* last arrival time */
	} t;

	long number = 0; /* Number in the network */

	t.current = START; /* set the clock */
	t.arrival = getArrivalStatic(); /* schedule the first arrival */

	//Initialize nodes
	for (int j = 0; j < 4; j++) {
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

	for (int i = 0; i < n0; i++) {
		servers[0][i].open = 1;
		servers[0][i].stream = current_stream;
		current_stream++;
	}
	for (int i = 0; i < n1; i++) {
		servers[1][i].open = 1;
		servers[1][i].stream = current_stream;
		current_stream++;
	}
	for (int i = 0; i < n2; i++) {
		servers[2][i].open = 1;
		servers[2][i].stream = current_stream;
		current_stream++;
	}
	for (int i = 0; i < n3; i++) {
		servers[3][i].open = 1;
		servers[3][i].stream = current_stream;
		current_stream++;
	}

	int first_class_number = 0;
	int second_class_number = 0;
	int tot_arrivals = 0;

	int id;
	int type;
	int dest_type;

	while ((statistics && tot_completions < N) ||
	       (!statistics && t.arrival < STOP)) {
		//while (tot_completions < N) {
		double minCompletion = minNode(servers, &id, &type);

		t.next = min(t.arrival, minCompletion);

		if (statistics) {
			for (int j = 0; j < 4; j++) {
				for (int i = 0; i < nodesNumber; i++) {
					if (servers[j][i].number >
					    0) { // update integrals
						servers[j][i].area.node +=
							(t.next - t.current) *
							servers[j][i].number;
						servers[j][i].area.queue +=
							(t.next - t.current) *
							(servers[j][i].number -
							 1);
						servers[j][i].area.service +=
							(t.next - t.current);
					}
				}
			}
		}

		t.current = t.next;

		if (t.current == t.arrival) {
			number++;
			tot_arrivals++;
			int p_type = getPassenger();

			int destination =
				getDestination(TEMP, &dest_type, mode, p_type);

			enqueue(&servers[dest_type][destination].head,
				&servers[dest_type][destination].tail, p_type,
				t.arrival);
			t.arrival = getArrivalStatic();

			servers[dest_type][destination].number++;

			if (p_type == FIRST_CLASS)
				servers[dest_type][destination].number1++;
			else
				servers[dest_type][destination].number2++;

			if (servers[dest_type][destination].number == 1)
				servers[dest_type][destination].completion =
					t.current +
					getService(dest_type, destination);

		} else {
			double arrival;
			int destination = 0;
			enum passenger_type pass_type;

			servers[type][id].number--;

			switch (type) {
			case TEMP:
				dequeue(&servers[type][id].head,
					&servers[type][id].tail, &pass_type,
					&arrival);

				if (pass_type == FIRST_CLASS)
					servers[type][id].number1--;
				else
					servers[type][id].number2--;

				if (servers[type][id].number > 0)
					servers[type][id].completion =
						t.current +
						getService(type, id);
				else
					servers[type][id].completion = INFINITY;

				destination = getDestination(CHECK, &dest_type,
							     mode, pass_type);

				if (destination != -1) {
					servers[dest_type][destination].number++;

					if (mode == 0 ||
					    pass_type == FIRST_CLASS) {
						enqueue(&servers[dest_type]
								[destination]
									.head,
							&servers[dest_type]
								[destination]
									.tail,
							pass_type, arrival);
						servers[dest_type][destination]
							.number1++;
					} else {
						enqueue(&servers[dest_type]
								[destination]
									.head_second,
							&servers[dest_type]
								[destination]
									.tail_second,
							pass_type, arrival);
						servers[dest_type][destination]
							.number2++;
					}

					if (servers[dest_type][destination]
						    .number == 1) {
						servers[dest_type][destination]
							.completion =
							t.current +
							getService(dest_type,
								   destination);
						servers[dest_type][destination]
							.service_type =
							pass_type;
					}
				} else {
					number--;
				}

				break;
			case DROP_OFF:
			case CHECK:
				if (mode == 0 ||
				    (servers[type][id].head != NULL &&
				     servers[type][id].service_type ==
					     FIRST_CLASS)) {
					dequeue(&servers[type][id].head,
						&servers[type][id].tail,
						&pass_type, &arrival);
				} else if (mode == 1 &&
					   (servers[type][id].head == NULL ||
					    servers[type][id].service_type !=
						    FIRST_CLASS)) {
					//} else {
					dequeue(&servers[type][id].head_second,
						&servers[type][id].tail_second,
						&pass_type, &arrival);
				}

				if (pass_type == FIRST_CLASS) {
					servers[type][id].number1--;
				} else {
					servers[type][id].number2--;
					//printf("CHOSE SECOND CLASS\n");
				}
				if (servers[type][id].number > 0) {
					servers[type][id].completion =
						t.current +
						getService(type, id);
					if (servers[type][id].head != NULL)
						servers[type][id].service_type =
							FIRST_CLASS;
					else
						servers[type][id].service_type =
							SECOND_CLASS;
				} else
					servers[type][id].completion = INFINITY;

				destination = getDestination(
					SECURITY, &dest_type, mode, pass_type);

				servers[dest_type][destination].number++;
				if (mode == 0 || pass_type == FIRST_CLASS) {
					enqueue(&servers[dest_type][destination]
							 .head,
						&servers[dest_type][destination]
							 .tail,
						pass_type, arrival);

					servers[dest_type][destination]
						.number1++;
				} else {
					enqueue(&servers[dest_type][destination]
							 .head_second,
						&servers[dest_type][destination]
							 .tail_second,
						pass_type, arrival);
					servers[dest_type][destination]
						.number2++;
				}

				if (servers[dest_type][destination].number ==
				    1) {
					servers[dest_type][destination]
						.completion =
						t.current +
						getService(dest_type,
							   destination);
					servers[dest_type][destination]
						.service_type = pass_type;
				}
				break;
			case SECURITY:
				number--;
				if (mode == 0 ||
				    (servers[type][id].head != NULL &&
				     servers[type][id].service_type ==
					     FIRST_CLASS)) {
					dequeue(&servers[type][id].head,
						&servers[type][id].tail,
						&pass_type, &arrival);
				} else if (mode == 1 &&
					   (servers[type][id].head == NULL ||
					    servers[type][id].service_type !=
						    FIRST_CLASS)) {
					//} else {
					dequeue(&servers[type][id].head_second,
						&servers[type][id].tail_second,
						&pass_type, &arrival);
				}

				if (pass_type == FIRST_CLASS) {
					servers[type][id].number1--;
				} else {
					servers[type][id].number2--;
				}

				SelectStream(255);
				double time_airport =
					Normal(TIME_IN_AIRPORT, VARIANCE);
				double response_time = (t.current - arrival);
				printf("Response time %lf type %d\n",
				       response_time, pass_type);
				int spending;

				if (statistics) {
					//Welford
					batch_position++;
					double diff =
						response_time - batch_mean;
					batch_mean += diff / batch_position;

					//Wellford for second class
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

							batch_position_second_class =
								0;
							batch_second_class_mean =
								0;
						}
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

							batch_position_first_class =
								0;
							batch_first_class_mean =
								0;
						}
						first_class_number++;
					}

					if (batch_position == BATCH_SIZE + 1) {
						//Total Mean Welford
						batches++;
						diff = batch_mean - mean;
						variance += diff * diff *
							    ((batches - 1.0) /
							     batches);
						mean += diff / batches;

						batch_position = 0;
						batch_mean = 0;
					}
				}

				if (pass_type == FIRST_CLASS) {
					spending = FIRST_CLASS_SPENDING;
				} else {
					spending = SECOND_CLASS_SPENDING;
				}

				tot_completions++;

				double time_left = time_airport - response_time;
				if (statistics)
					fprintf(samples, "%lf\n",
						response_time);

				if (time_left > 0) {
					income += (time_left / time_airport) *
						  spending;
				}

				if (servers[type][id].number > 0) {
					servers[type][id].completion =
						t.current +
						getService(type, id);
					if (servers[type][id].head != NULL)
						servers[type][id].service_type =
							FIRST_CLASS;
					else
						servers[type][id].service_type =
							SECOND_CLASS;
				} else
					servers[type][id].completion = INFINITY;

				break;
			}
		}
	}

	for (int j = 0; j < 4; j++) {
		int max_servers;

		switch (j) {
		case TEMP:
			max_servers = n0;
			break;
		case CHECK:
			max_servers = n1;
			break;
		case SECURITY:
			max_servers = n2;
			break;
		case DROP_OFF:
			max_servers = n3;
			break;
		}

		for (int i = 0; i < max_servers; i++) {
			cost += SERV_COST * t.current;
		}
	}

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < MAX_SERVERS; i++) {
			remove_all(&servers[j][i].head);
			remove_all(&servers[j][i].head_second);
		}
	}

	if (statistics) {
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
		first_class_variance =
			first_class_variance / batches_first_class;
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
		second_class_variance =
			second_class_variance / batches_second_class;
		second_class_stdev = sqrt(second_class_variance);

		double t_student_second_class =
			idfStudent(batches_second_class - 1, 1 - ALFA / 2);
		double second_class_endpoint = t_student_second_class *
					       second_class_stdev /
					       sqrt(batches_second_class - 1);

		ets[0] = mean;
		ets[1] = endpoint;

		ets1[0] = first_class_mean;
		ets1[1] = first_class_endpoint;

		ets2[0] = second_class_mean;
		ets2[1] = second_class_endpoint;

		fclose(samples);
	}

	income -= cost;
	*inc = income;

	printf("\nUTILIZATION:\n");
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 25; i++) {
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
			printf("\t%s-%d-Open: %d......utilization: %lf, average population: %lf, %d\n",
			       type_string, i, servers[j][i].open,
			       servers[j][i].area.service / t.current,
			       servers[j][i].area.node / t.current,
			       servers[j][i].number);
		}
	}

	return 0;
}

int infinite_horizon(int mode)
{
	double inc;
	cost = 0;

	double ets[2];
	double ets1[2];
	double ets2[2];

	simulate(1, mode, TEMP_NODE, CHECK_NODE, SECURITY_NODE, DROPOFF_ONLINE,
		 &inc, ets, ets1, ets2);
	printf("INCOME: %lf\nETS: %lf\nETS1: %lf\nETS2: %lf\n", inc, ets[0],
	       ets1[0], ets2[0]);

	return 0;
}

void finite_horizon(int mode, int n0[3], int n1[3], int n2[3], int n3[3],
		    double final_income[3], double total_income[3],
		    double costs[3])
{
	int fd1 = open("finite_horizon_response_time.csv",
		       O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (fd1 == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	st_file = fdopen(fd1, "w");

	fprintf(st_file, "TIME,RESPONSETIME\n");

	struct {
		double arrival; /* next arrival time */
		double current; /* current time */
		double next; /* next (most imminent) event time */
		double last; /* last arrival time */
	} t;

	long number = 0; /* Number in the network */
	double ets = 0;
	double ets1 = 0;
	double ets2 = 0;

	double completions = 0;
	double completions1 = 0;
	double completions2 = 0;

	double prev_active_time[4][246];

	arr = 0;
	t.current = START; /* set the clock */
	t.arrival = getArrival(t.current); /* schedule the first arrival */

	//Initialize nodes
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < MAX_SERVERS; i++) {
			servers[j][i].completion = INFINITY;
			servers[j][i].head = NULL;
			servers[j][i].tail = NULL;
			servers[j][i].head_second = NULL;
			servers[j][i].tail_second = NULL;
			servers[j][i].id = i;
			servers[j][i].number = 0;
			servers[j][i].number1 = 0;
			servers[j][i].number2 = 0;

			servers[j][i].open = 0;

			servers[j][i].area.node = 0;
			servers[j][i].area.queue = 0;
			servers[j][i].area.service = 0;
		}
	}

	//Set starting servers
	int current_stream = 0;

	for (int i = 0; i < MAX_TEMP; i++) {
		if (i < n0[0]) {
			servers[0][i].open = 1;
			servers[0][i].opening_time = 0;
		}
		servers[0][i].stream = current_stream;
		current_stream++;
	}
	for (int i = 0; i < MAX_CHECK; i++) {
		if (i < n1[0]) {
			servers[1][i].open = 1;
			servers[1][i].opening_time = 0;
		}
		servers[1][i].stream = current_stream;
		current_stream++;
	}
	for (int i = 0; i < MAX_SECURITY; i++) {
		if (i < n2[0]) {
			servers[2][i].open = 1;
			servers[2][i].opening_time = 0;
		}
		servers[2][i].stream = current_stream;
		current_stream++;
	}
	for (int i = 0; i < MAX_DROP_OFF; i++) {
		if (i < n3[0]) {
			servers[3][i].open = 1;
			servers[3][i].opening_time = 0;
		}
		servers[3][i].stream = current_stream;
		current_stream++;
	}

	int id;
	int type;
	int dest_type;

	int turn_change = 360;

	while ((t.arrival < STOP_FINITE)) {
		double minCompletion = minNode(servers, &id, &type);

		t.next = min(t.arrival, minCompletion);
		t.next = min(t.next, turn_change);

		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < 246; i++) {
				if (servers[j][i].number >
				    0) { // update integrals
					servers[j][i].area.node +=
						(t.next - t.current) *
						servers[j][i].number;
					servers[j][i].area.queue +=
						(t.next - t.current) *
						(servers[j][i].number - 1);
					servers[j][i].area.service +=
						(t.next - t.current);

					//TODO ok???
					//If is closed but has still someone in the queue add active times
					if (!servers[j][i].open) {
						servers[j][i].active_time +=
							(t.next - t.current);
					}
				}
			}
		}

		t.current = t.next;

		if (t.current == t.arrival) {
			number++;

			int p_type = getPassenger();

			int destination =
				getDestination(TEMP, &dest_type, mode, p_type);

			enqueue(&servers[dest_type][destination].head,
				&servers[dest_type][destination].tail, p_type,
				t.arrival);
			t.arrival = getArrival(t.current);

			servers[dest_type][destination].number++;
			if (p_type == FIRST_CLASS)
				servers[dest_type][destination].number1++;
			else
				servers[dest_type][destination].number2++;

			if (t.arrival > STOP) {
				t.last = t.current;
				t.arrival = INFINITY;
			}

			if (servers[dest_type][destination].number == 1)
				servers[dest_type][destination].completion =
					t.current +
					getService(
						servers[dest_type][destination]
							.type,
						destination);

		} else if (t.current == turn_change) {
			switch (turn_change) {
			case 360:
				turn_change = 1080;
				change_servers(0, n0[1], t.current);
				change_servers(1, n1[1], t.current);
				change_servers(2, n2[1], t.current);
				change_servers(3, n3[1], t.current);
				total_income[0] = income;
				for (int j = 0; j < 4; j++) {
					for (int i = 0; i < 246; i++) {
						cost += SERV_COST *
							servers[j][i]
								.active_time;
						prev_active_time[j][i] =
							servers[j][i]
								.active_time;
					}
				}
				costs[0] = cost;
				final_income[0] = income - cost;
				income = 0;
				cost = 0;
				break;
			case 1080:
				change_servers(0, n0[2], t.current);
				change_servers(1, n1[2], t.current);
				change_servers(2, n2[2], t.current);
				change_servers(3, n3[2], t.current);
				total_income[1] = income;
				for (int j = 0; j < 4; j++) {
					for (int i = 0; i < 246; i++) {
						cost += SERV_COST *
							(servers[j][i]
								 .active_time -
							 prev_active_time[j][i]);
						prev_active_time[j][i] =
							servers[j][i]
								.active_time;
					}
				}
				costs[1] = cost;
				final_income[1] = income - cost;
				income = 0;
				cost = 0;
				turn_change = 10 * STOP_FINITE;
				break;
			default:
				turn_change = 10 * STOP_FINITE;
				break;
			}

		} else {
			double arrival;
			int destination = 0;
			enum passenger_type pass_type;

			servers[type][id].number--;

			switch (type) {
			case TEMP:
				dequeue(&servers[type][id].head,
					&servers[type][id].tail, &pass_type,
					&arrival);

				if (pass_type == FIRST_CLASS)
					servers[type][id].number1--;
				else
					servers[type][id].number2--;

				if (servers[type][id].number > 0)
					servers[type][id].completion =
						t.current +
						getService(type, id);
				else
					servers[type][id].completion = INFINITY;

				destination = getDestination(CHECK, &dest_type,
							     mode, pass_type);
				if (destination != -1) {
					servers[dest_type][destination].number++;

					if (mode == 0 ||
					    pass_type == FIRST_CLASS) {
						enqueue(&servers[dest_type]
								[destination]
									.head,
							&servers[dest_type]
								[destination]
									.tail,
							pass_type, arrival);
						servers[dest_type][destination]
							.number1++;

					} else {
						enqueue(&servers[dest_type]
								[destination]
									.head_second,
							&servers[dest_type]
								[destination]
									.tail_second,
							pass_type, arrival);

						servers[dest_type][destination]
							.number2++;
					}

					if (servers[dest_type][destination]
						    .number == 1) {
						servers[dest_type][destination]
							.completion =
							t.current +
							getService(dest_type,
								   destination);
						servers[dest_type][destination]
							.service_type =
							pass_type;
					}
				} else {
					number--;
				}

				break;
			case DROP_OFF:
			case CHECK:
				if (mode == 0 ||
				    (servers[type][id].head != NULL &&
				     servers[type][id].service_type ==
					     FIRST_CLASS)) {
					dequeue(&servers[type][id].head,
						&servers[type][id].tail,
						&pass_type, &arrival);
				} else {
					dequeue(&servers[type][id].head_second,
						&servers[type][id].tail_second,
						&pass_type, &arrival);
				}

				if (pass_type == FIRST_CLASS)
					servers[type][id].number1--;
				else
					servers[type][id].number2--;

				if (servers[type][id].number > 0) {
					servers[type][id].completion =
						t.current +
						getService(type, id);
					if (servers[type][id].head != NULL)
						servers[type][id].service_type =
							FIRST_CLASS;
					else
						servers[type][id].service_type =
							SECOND_CLASS;
				} else
					servers[type][id].completion = INFINITY;

				destination = getDestination(
					SECURITY, &dest_type, mode, pass_type);

				servers[dest_type][destination].number++;
				if (mode == 0 || pass_type == FIRST_CLASS) {
					enqueue(&servers[dest_type][destination]
							 .head,
						&servers[dest_type][destination]
							 .tail,
						pass_type, arrival);
					servers[dest_type][destination]
						.number1++;
				} else {
					enqueue(&servers[dest_type][destination]
							 .head_second,
						&servers[dest_type][destination]
							 .tail_second,
						pass_type, arrival);

					servers[dest_type][destination]
						.number2++;
				}

				if (servers[dest_type][destination].number ==
				    1) {
					servers[dest_type][destination]
						.completion =
						t.current +
						getService(type, destination);
					servers[dest_type][destination]
						.service_type = pass_type;
				}

				break;
			case SECURITY:
				number--;

				if (mode == 0 ||
				    (servers[type][id].head != NULL &&
				     servers[type][id].service_type ==
					     FIRST_CLASS)) {
					dequeue(&servers[type][id].head,
						&servers[type][id].tail,
						&pass_type, &arrival);
				} else {
					dequeue(&servers[type][id].head_second,
						&servers[type][id].tail_second,
						&pass_type, &arrival);
				}

				if (pass_type == FIRST_CLASS)
					servers[type][id].number1--;
				else
					servers[type][id].number2--;

				double time_airport =
					Normal(TIME_IN_AIRPORT, VARIANCE);
				double response_time = t.current - arrival;

				fprintf(st_file, "%lf,%lf\n", t.current,
					response_time);

				double spending;
				if (pass_type == FIRST_CLASS) {
					ets1 += response_time;
					completions1++;
					spending = FIRST_CLASS_SPENDING;
				} else {
					ets2 += response_time;
					completions2++;
					spending = SECOND_CLASS_SPENDING;
				}

				double time_left = time_airport - response_time;
				if (time_left > 0) {
					income += (time_left / time_airport) *
						  spending;
				}

				ets += response_time;
				completions++;

				if (servers[type][id].number > 0) {
					servers[type][id].completion =
						t.current +
						getService(type, id);
					if (servers[type][id].head != NULL)
						servers[type][id].service_type =
							FIRST_CLASS;
					else
						servers[type][id].service_type =
							SECOND_CLASS;
				} else
					servers[type][id].completion = INFINITY;

				break;
			}
		}
	}

	t.next = STOP_FINITE;

	ets = ets / completions;
	ets1 = ets1 / completions1;
	ets2 = ets2 / completions2;

	total_income[2] = income;
	for (int j = 0; j < 4; j++) {
		change_servers(j, 0, STOP_FINITE);
		for (int i = 0; i < 246; i++) {
			cost += SERV_COST * (servers[j][i].active_time -
					     prev_active_time[j][i]);
		}
	}
	costs[2] = cost;
	final_income[2] = income - cost;

	//cost and income are global reset for next repetition
	cost = 0;
	income = 0;

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < MAX_SERVERS; i++) {
			remove_all(&servers[j][i].head);
			remove_all(&servers[j][i].head_second);
		}
	}

	fclose(st_file);

	printf("RESPONSE TIME: %lf COMPLETIONS: %0.f\n", ets, completions);
	printf("RESPONSE TIME 1: %lf COMPLETIONS: %0.f\n", ets1, completions1);
	printf("RESPONSE TIME 2: %lf COMPLETIONS: %0.f\n", ets2, completions2);
}

int repeat_infinite_horizon()
{
	double max_income = -INFINITY;
	int opt_t = 0;
	int opt_c = 0;
	int opt_s = 0;
	int opt_d = 0;

	double temp_income;
	double ets[2];
	double ets1[2];
	double ets2[2];

	for (int m = 0; m < 2; m++) {
		printf("Mode %d\n", m);

		for (int t = 1; t < MAX_TEMP; t++) {
			printf("T: %d\n", t);
			for (int c = 1; c < MAX_CHECK; c++) {
				printf("C: %d\n", c);
				for (int s = 1; s < MAX_SECURITY; s++) {
					for (int d = 1; d < MAX_DROP_OFF; d++) {
						income = 0;
						cost = 0;

						PlantSeeds(SEED);
						simulate(0, m, t, c, s, d,
							 &temp_income, ets,
							 ets1, ets2);
						//printf("Income %d-%d-%d-%d: %lf\n", t,c,s,d,temp_income);
						if (temp_income > max_income) {
							max_income =
								temp_income;
							opt_t = t;
							opt_c = c;
							opt_s = s;
							opt_d = d;

							//printf("New Optimum:%d-%d-%d-%d %lf", opt_t, opt_c, opt_s, opt_d, max_income);
						}
					}
				}
			}
		}

		printf("OPTIMUM mode:%d for %d-%d-%d-%d: with %lf\n", m, opt_t,
		       opt_c, opt_s, opt_d, max_income);

		max_income = -INFINITY;
	}

	return 0;
}

int finite_horizon_single(int mode)
{
	int *n0 = finite_temp_node;
	int *n1 = finite_check_node;
	int *n2 = finite_security_node;
	int *n3 = finite_dropoff_node;

	double total_income[3];
	double final_income[3];
	double costs[3];

	finite_horizon(mode, n0, n1, n2, n3, final_income, total_income, costs);

	double income_sum = 0;
	double cost_sum = 0;
	double earning_sum = 0;

	for (int i = 0; i < 3; i++) {
		printf("Phase %d->income: %lf costs: %lf earning: %lf \n",
		       i + 1, total_income[i], costs[i], final_income[i]);

		income_sum += total_income[i];
		cost_sum += costs[i];
		earning_sum += final_income[i];
	}

	printf("TOTAL->income: %lf costs: %lf earning: %lf \n", income_sum,
	       cost_sum, earning_sum);

	return 0;
}

int main(int argc, char **argv)
{
	int mode;

	if (argc != 3) {
		printf("Usage: simulation <mode> (o original, rp improved routing and priority, r improved routing, p improved priority) <finite-infinite> (f finite, i infinite, r find optimum)\n");
		return -1;
	}

	PlantSeeds(SEED);

	if (!strcmp(argv[1], "o")) {
		mode = 0;
	} else if (!strcmp(argv[1], "r")) {
		mode = 2;
	} else if (!strcmp(argv[1], "p")) {
		mode = 1;
	} else if (!strcmp(argv[1], "rp")) {
		mode = 3;
	} else {
		printf("Usage: simulation <mode> (o original, rp improved routing and priority, r improved routing, p improved priority) <finite-infinite> (f finite, i infinite, r find optimum)\n");
		return -1;
	}

	if (!strcmp(argv[2], "r")) {
		return repeat_infinite_horizon();
	} else if (!strcmp(argv[2], "i")) {
		return infinite_horizon(mode);
	} else if (!strcmp(argv[2], "f")) {
		return finite_horizon_single(mode);
	} else {
		printf("Usage: simulation <mode> (o original, rp improved routing and priority, r improved routing, p improved priority) <finite-infinite> (f finite, i infinite, r find optimum)\n");
		return -1;
	}
}
