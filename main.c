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
#include  <signal.h>

#include "route_cfg_parser.h"
#include "main.h"

#define BUFLEN 512

#define SRV_IP "127.0.0.1"

bool verbosity = true; // should be passed as 3rd argument // already is (optional)
char quiet[100] = "--quiet";
int connectionCount = 100;
int localPort;
TConnection connections[100];
int localId;

char inputMsg[128];

RoutingTableItem routingTable[100];
char clientMessage[100] = "connected ";
char sendingMessage[100] = "message ";

volatile sig_atomic_t got_sig;

int toString(char []);

//handling the sigint signal to terminate
void sigintHandler(int sig) {
    got_sig = 1;
    //printf("Signal received.\n");
}

void verbPrintf(const char *format, ...) {
    // va_list is a special type that allows handling of variable
    // length parameter list
    va_list args;
    va_start(args, format);

    // If verbosity flag is on then print it
    if (verbosity)
        vfprintf(stderr, format, args);
    // Do nothing
}

void diep(char *s) {
    perror(s);
    exit(1);
}


void *clientThread(void *arg) {
    int i = 0;


    struct sockaddr_in si_other;
    struct sockaddr_in sis[100];
    int s, slen = sizeof(si_other);
    char buf[BUFLEN];
    char wholeMessage[BUFLEN];

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    int ii;
    for (ii = 0; ii < connectionCount; ii++) {
        memset((char *) &sis[ii], 0, sizeof(sis[ii]));
        sis[ii].sin_family = AF_INET;
        sis[ii].sin_port = htons(connections[ii].port);
        if (inet_aton(SRV_IP, &sis[ii].sin_addr) == 0) {
            fprintf(stderr, "inet_aton() failed\n");
            exit(1);
        }
    }


    while (!got_sig) {
//        sleep(1);
        printf("Sending packet %d\n", i);
//        sprintf(buf, "This is packet %d\n", i);
        strcpy(wholeMessage, clientMessage);
        char str[15];
        sprintf(str, "%d", localPort);
        strcat(wholeMessage, str);
        sprintf(buf, clientMessage);
        for (ii = 0; ii < connectionCount; ii++) {
            if (sendto(s, buf, BUFLEN, 0, &sis[ii], slen) == -1) {
                diep("sendto()");
            }

            verbPrintf("Send packet to %s:%d\nData: %s\n\n", inet_ntoa(sis[ii].sin_addr), ntohs(sis[ii].sin_port), buf);
        }

        i = i + 1 % 200000;
        sleep(1);
    }
    close(s);
    return NULL;
}

void *serverThread(void *arg) {
    char *str;
    int i = 0;

    str = (char *) arg;
    struct sockaddr_in si_me, si_other;
    int s, slen = sizeof(si_other);
    char buf[BUFLEN];

    //variables used to separate parts of the message
    char *nodeNum;
    char wholeMessage[128];//all three parts
    char *msgPart;
    int receivingNode;

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(localPort);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, &si_me, sizeof(si_me)) == -1)
        diep("bind");

    while (!got_sig) {
        sleep(1);
        if (recvfrom(s, buf, BUFLEN, 0, &si_other, &slen) == -1)
            diep("recvfrom()");

        strcpy(wholeMessage, buf);


        printf("whole message before converting: %s\n", wholeMessage);

        strtok_r(wholeMessage, " ", &nodeNum);

        if (strcmp(wholeMessage, clientMessage) == 0) {
            verbPrintf("Received CONNECTING packet from %s:%d\nMessage: %s\n\n",
                       inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), wholeMessage);
        } else {

            strtok_r(nodeNum, " ", &msgPart);
            receivingNode = atoi(nodeNum);

            printf("whole message: %s\n", wholeMessage);
            printf("nodeNum: %s\n", nodeNum);
            printf("msgPart: %s\n", msgPart);

            if (receivingNode == localId) {
                printf("Received packet from %s:%d\nMessage: %s\n\n",
                       inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), msgPart);
            } else {
                //send buf (the whole message) to proper node

            }
        }

        //printf("Received packet from %s:%d\nData: %s\n\n",
        //       inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

        ++i;
    }
    close(s);
    return NULL;
}

//converts char array to string
int toString(char a[]) {
    int c, sign, offset, n;

    if (a[0] == '-') {  // Handle negative integers
        sign = -1;
    }

    if (sign == -1) {  // Set starting position to convert
        offset = 1;
    }
    else {
        offset = 0;
    }

    n = 0;

    for (c = offset; a[c] != '\0'; c++) {
        n = n * 10 + a[c] - '0';
    }

    if (sign == -1) {
        n = -n;
    }

    return n;
}


int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\nUsage: %s <ID> [ <cfg-file-name> ]\n", argv[0]);
        return -1;
    }
    if (argc > 3) {
        if (strcmp(argv[3], quiet) == 0) {
            verbosity = false;
        }

    }

    //handling the signal from CTRL+C
    struct sigaction sig_action;
    got_sig = 0;

    sig_action.sa_handler = sigintHandler;
    sig_action.sa_flags = 0;
    sigemptyset(&sig_action.sa_mask);

    if (sigaction(SIGINT, &sig_action, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    localId = atoi(argv[1]);
    verbPrintf("\n%s\n", argv[2]);
    int result = parseRouteConfiguration((argc > 2) ? argv[2] : NULL, localId, &localPort, &connectionCount,
                                         connections);
    if (result) {
        verbPrintf("OK, local port: %d\n", localPort);

        if (connectionCount > 0) {
            int i;
            for (i = 0; i < connectionCount; i++) {
                verbPrintf("Connection to node %d at %s%s%d\n", connections[i].id, connections[i].ip_address,
                           (connections[i].ip_address[0]) ? ":" : "port ", connections[i].port);
            }
        } else {
            verbPrintf("No connections from this node\n");
        }
    } else {
        verbPrintf("ERROR\n");
    }

    //creating server and client thread
    pthread_t pthreadClient;    // this is our thread identifier
    int i = 0;

    pthread_create(&pthreadClient, NULL, clientThread, "foo");

    pthread_t pthreadServer;

    pthread_create(&pthreadServer, NULL, serverThread, "foo");

    printf("main is ready to run...\n");
    printf("main waiting for thread to terminate...\n");

    printf("Ready to send messages.");

    //accepting messages from stdin

    struct sockaddr_in si_other;
    int s, slen = sizeof(si_other);
    char buf[BUFLEN];
    int receivingNode;
    int j;

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    //reading and sending msgs
    while (!got_sig) {
        if (fgets(inputMsg, 128, stdin) != NULL) {

            //sending message to desired port
            printf("Message to send: %s\n", inputMsg);

            //separate parts of the message
            char nodeNum[128];// also msgPart2
            char *msgPart3;
            char wholeMessage[128];
            strcpy(wholeMessage, sendingMessage);
            strcpy(nodeNum, inputMsg);
            strtok_r(nodeNum, " ", &msgPart3);
            printf("Part 1: %s ; Part 2: %s \n", nodeNum, msgPart3);
            printf("Receiving node is going to be: %s\n.", nodeNum);
            //choose converter or atoi ??
            //receivingNode = toString(nodeNum);
            receivingNode = atoi(nodeNum);
            //printf("Receiving node is going to be (converted): %d\n.", receivingNode);

            si_other.sin_family = AF_INET;

            //find the port
            for (j = 0; j < connectionCount; j++) {
                if (connections[j].id == receivingNode) {
                    si_other.sin_port = htons(connections[j].port);
                    break;
                }
            }

            if (inet_aton(SRV_IP, &si_other.sin_addr) == 0) {
                fprintf(stderr, "inet_aton() failed\n");
                exit(1);
            }

            strcat(wholeMessage, inputMsg);

//            sprintf(buf, "%s\n", inputMsg);
            sprintf(buf, "%s\n", wholeMessage);
            if (sendto(s, buf, BUFLEN, 0, &si_other, slen) == -1)
                diep("sendto()");
            printf("Send message to %s:%d\nData: %s\n\n",
                   inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);
        }
    }

    pthread_join(pthreadClient, NULL);
    exit(0);
}