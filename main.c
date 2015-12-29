//
// Created by marek on 28.12.15.
//

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "route_cfg_parser.h"

bool verbosity = true; // should be passed as 3rd argument
char quiet[10] = "--quiet";

void verbPrintf(const char *format, ...)
{
    // va_list is a special type that allows handling of variable
    // length parameter list
    va_list args;
    va_start(args, format);

    // If verbosity flag is on then print it
    if (verbosity)
        vfprintf (stderr, format, args);
    // Do nothing
}

void *threadFunc(void *arg)
{
    char *str;
    int i = 0;

    str=(char*)arg;

    while(i < 110 )
    {
        usleep(1);
        printf("threadFunc says: %s\n",str);
        ++i;
    }

    return NULL;
}


int main(int argc, char ** argv) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\nUsage: %s <ID> [ <cfg-file-name> ]\n", argv[0]);
        return -1;
    }
    if (argc > 3) {
        if(strcmp(argv[3],quiet) == 0){
            verbosity = false;
        }

    }
    int localId = atoi(argv[1]);
    int connectionCount = 100;
    int localPort;
    TConnection connections[connectionCount];
    verbPrintf("\n%s\n",argv[2]);
    int result = parseRouteConfiguration((argc>2)?argv[2]:NULL, localId, &localPort, &connectionCount, connections);
    if (result) {
        verbPrintf("OK, local port: %d\n", localPort);

        if (connectionCount > 0) {
            int i;
            for (i=0; i<connectionCount; i++) {
                verbPrintf("Connection to node %d at %s%s%d\n", connections[i].id, connections[i].ip_address, (connections[i].ip_address[0])?":":"port ", connections[i].port);
            }
        } else {
            verbPrintf("No connections from this node\n");
        }
    } else {
        verbPrintf("ERROR\n");
    }

    pthread_t pth;	// this is our thread identifier
    int i = 0;

    pthread_create(&pth,NULL,threadFunc,"foo");

    while(i < 100)
    {
        usleep(1);
        printf("main is running...\n");
        ++i;
    }

    printf("main waiting for thread to terminate...\n");
    pthread_join(pth,NULL);
}