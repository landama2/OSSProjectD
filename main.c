//
// Created by marek on 28.12.15.
//

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "route_cfg_parser.h"

bool verbosity = false;

void verbPrintf(const char *message) {

    if (verbosity)
        printf(message);
    else {

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
        printf("OK, local port: %d\n", localPort);

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