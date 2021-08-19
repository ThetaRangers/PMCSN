.PHONY: simulation clean
CROSS_COMPILE ?= gcc
LIBS = -lm
CFLAGS = -Wall -Wextra -O3
simulation:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -o simulation
clean:
	rm -f simulation
