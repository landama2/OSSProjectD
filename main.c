//
// Created by marek on 28.12.15.
//

#include <stdio.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "route_cfg_parser.h"

bool verbosity = false;

void verbPrintf(const char *format, ...)
{
    // va_list is a special type that allows hanlding of variable
    // length parameter list
    va_list args;
    va_start(args, format);

    // If verbosity flag is on then print it
    if (verbosity)
        vfprintf (stdout, format, args);
    else{

    }
    // Do nothing
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\nUsage: %s <ID> [ <cfg-file-name> ]\n", argv[0]);
        return -1;
    }
    int localId = atoi(argv[1]);
    int connectionCount = 100;
    int localPort;
    TConnection connections[connectionCount];
    printf("\n%s\n",argv[2]);
    int result = parseRouteConfiguration((argc>2)?argv[2]:NULL, localId, &localPort, &connectionCount, connections);
    if (result) {
        verbPrintf("OK, local port: %d\n", localPort);

        if (connectionCount > 0) {
            int i;
            for (i=0; i<connectionCount; i++) {
                printf("Connection to node %d at %s%s%d\n", connections[i].id, connections[i].ip_address, (connections[i].ip_address[0])?":":"port ", connections[i].port);
            }
        } else {
            printf("No connections from this node\n");
        }
    } else {
        printf("ERROR\n");
    }
}