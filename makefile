C=gcc
CFLAGS= -std=c11 -Wall -g -pedantic
OBJECTS= simplesh.o

all: simplesh
simplesh: $(OBJECTS)
	$(CC) $(CFLAGS) -o simplesh $(OBJECTS)
simplesh.o: simplesh.c
	$(CC) -c $(CFLAGS) simplesh.c

.PHONY: clean
clean:
	rm *.o simplesh
