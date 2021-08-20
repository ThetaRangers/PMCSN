#include "utils.h"
#include "rngs.h" 
#include "rvgs.h"

double min(double a, double c)  { 
  if (a < c)
    return (a);
  else
    return (c);
}

enum passenger_type getPassenger() {
    long res = Bernoulli(0.3);

    if(res == 1) return FIRST_CLASS;

    return SECOND_CLASS;
}