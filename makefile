CC = gcc
LDFLAGS = -lpthread

default: program

program: main.o route_cfg_parser.o
	$(CC) -o program main.o route_cfg_parser.o $(LDFLAGS)

main.o: main.c
	$(CC) -c main.c $(LDFLAGS)

route_cfg_parser.o: route_cfg_parser.c route_cfg_parser.h
	$(CC) -c route_cfg_parser.c $(LDFLAGS)

clean:
	rm *.o
