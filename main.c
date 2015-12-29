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

#define BUFLEN 512
#define NPACK 10
#define PORT 9930

#define SRV_IP "127.0.0.1"

bool verbosity = true; // should be passed as 3rd argument
char quiet[10] = "--quiet";
int connectionCount = 100;
int localPort;
TConnection connections[100];
int localId;

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

void diep(char *s)
{
    perror(s);

    exit(1);
}

void *clientThread(void *arg)
{
    char *str;
    int i = 0;

    str=(char*)arg;
    struct sockaddr_in si_other;
    struct sockaddr_in sis[100];
    int s, slen=sizeof(si_other);
    char buf[BUFLEN];

    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        diep("socket");

    int ii;
    for (ii =0; ii < connectionCount; ii++) {
        memset((char *) &sis[ii], 0, sizeof(sis[ii]));
        sis[ii].sin_family = AF_INET;
        sis[ii].sin_port = htons(connections[ii].port);
        if (inet_aton(SRV_IP, &sis[ii].sin_addr) == 0) {
            fprintf(stderr, "inet_aton() failed\n");
            exit(1);
        }

//        verbPrintf("Connection to node %d at %s%s%d\n", connections[i].id, connections[i].ip_address, (connections[i].ip_address[0])?":":"port ", connections[i].port);
    }


    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    if (inet_aton(SRV_IP, &si_other.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    while(1 == 1)
    {
        usleep(1);
        printf("Sending packet %d\n", i);
        sprintf(buf, "This is packet %d\n", i);
        ii = 0;
        for (ii =0; ii < connectionCount; ii++) {
            if (sendto(s, buf, BUFLEN, 0, &sis[ii], slen) == -1)
                diep("sendto()");
            printf("Send packet to %s:%d\nData: %s\n\n",
                   inet_ntoa(sis[ii].sin_addr), ntohs(sis[ii].sin_port), buf);
        }
//        if (sendto(s, buf, BUFLEN, 0, &si_other, slen)==-1)
//            diep("sendto()");

        printf("threadFunc says: %s\n",str);
        i = i + 1 % 200000;
        sleep(1);
    }
    close(s);
    return NULL;
}

void *serverThread(void *arg)
{
    char *str;
    int i = 0;

    str=(char*)arg;
    struct sockaddr_in si_me, si_other;
    int s, slen=sizeof(si_other);
    char buf[BUFLEN];

    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, &si_me, sizeof(si_me))==-1)
        diep("bind");

    while(1==1)
    {
        usleep(1);
        if (recvfrom(s, buf, BUFLEN, 0, &si_other, &slen)==-1)
            diep("recvfrom()");
        printf("Received packet from %s:%d\nData: %s\n\n",
               inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

        ++i;
    }
    close(s);
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
    localId = atoi(argv[1]);
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

    pthread_t pthreadClient;	// this is our thread identifier
    int i = 0;

    pthread_create(&pthreadClient, NULL, clientThread, "foo");

    pthread_t pthreadServer;

    pthread_create(&pthreadServer, NULL, serverThread, "foo");

    printf("main is ready to run...\n");
    printf("main waiting for thread to terminate...\n");
    pthread_join(pthreadClient, NULL);
}