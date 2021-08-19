#include <stdio.h>
#include "rngs.h" 
#include "rvgs.h"

int main() {
	double test;
	SelectStream(0);                  /* select the default stream */
	PutSeed(1);
	test = Exponential(6);
	printf("Numero: %lf\n", test);
}