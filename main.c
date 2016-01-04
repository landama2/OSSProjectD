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
#define MAXCOST 128
#define LENGHTOFARRAY 100
#define TIMEOUT 5

bool verbosity = true; // should be passed as 3rd argument // already is (optional)
char quiet[LENGHTOFARRAY] = "--quiet";
int connectionCount = LENGHTOFARRAY;
int localPort;
TConnection connections[LENGHTOFARRAY];
int localId;

bool connectionsAvailable[LENGHTOFARRAY];

char inputMsg[128];

volatile RoutingTableItem routingTable[LENGHTOFARRAY];
char clientMessage[LENGHTOFARRAY] = "connected";
char sendingMessage[LENGHTOFARRAY] = "message";
char updateMessage[LENGHTOFARRAY] = "update";

volatile struct sockaddr_in sis[LENGHTOFARRAY];


volatile sig_atomic_t got_sig;

int toInt(char []);

//void createUpdateMessage(char *buf, char *wholeMessage, char *msgPart, int cost);
void createUpdateMessage(char *buf, char *wholeMessage, int msgPart, int cost);

//handling the sigint signal to terminate
void sigintHandler(int sig) {
    got_sig = 1;
    //printf("Signal received.\n");
}

void verbPrintf(const char *format, ...) {
    // va_list is a special type that  allows handling of variable
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
    int s, slen = sizeof(si_other);
    char buf[BUFLEN];
    char wholeMessage[BUFLEN];

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");


    strcpy(wholeMessage, clientMessage);
    char str[15];
    sprintf(str, " %d", localId);
    strcat(wholeMessage, str);
    sprintf(buf, wholeMessage);

    while (!got_sig) {
//        sleep(1);
        //printf("Sending packet %d\n", i);
//        sprintf(buf, "This is packet %d\n", i);

        int ii;
        for (ii = 0; ii < connectionCount; ii++) {
            if (sendto(s, buf, BUFLEN, 0, &sis[ii], slen) == -1) {
                diep("sendto()");
            }

            verbPrintf("Send packet to %s:%d\nData: %s\n\n", inet_ntoa(sis[ii].sin_addr), ntohs(sis[ii].sin_port), buf);
        }

        i = i + 1 % 200000;

        int d;
        for (d = 0; d < LENGHTOFARRAY; d++) {
            connections[d].secSinceLastPacket++;
        }


        for (ii = 0; ii < LENGHTOFARRAY; ii++) {
            if (connectionsAvailable[ii]) {
//                connections[ii].secSinceLastPacket++;
//                if (connections[ii].secSinceLastPacket > TIMEOUT) {
                    connectionsAvailable[ii] = false;
                    int k;
                    for (k = 0; k < LENGHTOFARRAY; k++) {


                        int e;
                        for (e = 0; e < LENGHTOFARRAY; e++) {
                            if (connections[e].secSinceLastPacket > TIMEOUT) {
                               if (routingTable[k].idOfTargetNode == connections[e].id || routingTable[k].idOfNextNode == connections[e].id) {
                                if (routingTable[k].idOfTargetNode != 0) {
                                    routingTable[k].cost = MAXCOST;

                                    char buf2[BUFLEN];
                                    char wholeMessage2[BUFLEN];

                                    //!!
                                    strcpy(wholeMessage2, updateMessage);
                                    char str1[15];
                                    char str2[15];
                                    char strmsgPart[15];
                                    sprintf(str1, " %d", localId);
                                    strcat(wholeMessage2, str1);
                                    sprintf(strmsgPart, " %d", routingTable[k].idOfTargetNode);
                                    strcat(wholeMessage2, strmsgPart);
                                    sprintf(str2, " %d", MAXCOST);
                                    strcat(wholeMessage2, str2);
                                    sprintf(buf2, wholeMessage2);
                                    //!!

                                    verbPrintf(buf2);
                                    int f;
                                    for (f = 0; f < connectionCount; f++) {
                                        if (sendto(s, buf2, BUFLEN, 0, &sis[f], slen) == -1) {
                                            diep("sendto()");
                                        }

                                        verbPrintf("Send packet to %s:%d\nData: %s\n\n", inet_ntoa(sis[f].sin_addr),
                                                   ntohs(sis[f].sin_port), buf2);
                                    }
                                }
                            }
                            }
                        }
                    }


            }
        }
        sleep(1);
    }
    close(s);
    return NULL;
}

void *serverThread(void *arg) {
    int i = 0;

    struct sockaddr_in si_me, si_other;
    int s, slen = sizeof(si_other);
    char buf[BUFLEN];

    //variables used to separate parts of the message
    char *nodeNum;
    char wholeMessage[128];//all three parts
    char *msgPart;
    int receivedNode;

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(localPort);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, &si_me, sizeof(si_me)) == -1)
        diep("bind");

    while (!got_sig) {
        if (recvfrom(s, buf, BUFLEN, 0, &si_other, &slen) == -1)
            diep("recvfrom()");

        strcpy(wholeMessage, buf);

        //printf("whole message before converting: %s\n", wholeMessage);

        strtok_r(wholeMessage, " ", &nodeNum);

        strtok_r(nodeNum, " ", &msgPart);
        receivedNode = atoi(nodeNum);

        if (strcmp(wholeMessage, clientMessage) == 0) {
            //received connecting packet, not a message
            //nodeNum contains number and the message if it exists
            verbPrintf("Received CONNECTING packet from %s:%d\nMessage: %s %d\n\n",
                       inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), wholeMessage, receivedNode);
            //updating routing table
            routingTable[receivedNode].cost = 1;
            routingTable[receivedNode].idOfTargetNode = receivedNode;
            routingTable[receivedNode].idOfNextNode = receivedNode;

//            char str[15];
//            sprintf(str, " %d", receivedNode);
//            createUpdateMessage(buf, wholeMessage,str,1);

//            createUpdateMessage(buf, &wholeMessage,receivedNode,1);


            //!!


            strcpy(wholeMessage, updateMessage);
            char str[15];
            char str2[15];
            char strmsgPart[15];
            sprintf(str, " %d", localId);
            strcat(wholeMessage, str);
            sprintf(strmsgPart, " %d", receivedNode);
            strcat(wholeMessage, strmsgPart);
            sprintf(str2, " %d", 1);
            strcat(wholeMessage, str2);
            sprintf(buf, wholeMessage);
            //!!

            verbPrintf(buf);


            int ii;


//            int ii;
            for (ii = 0; ii < connectionCount; ii++) {
                if (connections[ii].id == receivedNode) {
                    connections[ii].secSinceLastPacket = 0;


                    if(!connectionsAvailable[receivedNode]) {
                        int j;
                        for (j = 0; j < connectionCount; j++) {

                            int a;
                            for (a = 0; a < LENGHTOFARRAY; a++) {
                                strcpy(wholeMessage, updateMessage);
                                char str[15];
                                char str2[15];
                                char strmsgPart[15];
                                sprintf(str, " %d", localId);
                                strcat(wholeMessage, str);
                                sprintf(strmsgPart, " %d", routingTable[a].idOfTargetNode);
                                strcat(wholeMessage, strmsgPart);
                                sprintf(str2, " %d", routingTable[a].cost);
                                strcat(wholeMessage, str2);
                                sprintf(buf, wholeMessage);

                                int b;
                                for (b = 0; b < connectionCount; b++) {
                                    if (sendto(s, buf, BUFLEN, 0, &sis[b], slen) == -1) {
                                        diep("sendto()");
                                    }

                                    verbPrintf("Send packet to %s:%d\nData: %s\n\n", inet_ntoa(sis[b].sin_addr),
                                               ntohs(sis[b].sin_port), buf);
                                }


                            }

//                            if (sendto(s, buf, BUFLEN, 0, &sis[j], slen) == -1) {
//                                diep("sendto()");
//                            }
//
//
//                            verbPrintf("Send packet to %s:%d\nData: %s\n\n", inet_ntoa(sis[j].sin_addr),
//                                       ntohs(sis[j].sin_port), buf);

                        }
                    }


                    connectionsAvailable[receivedNode] = true;
                }
            }

        } else if (strcmp(wholeMessage, updateMessage) == 0) {
            // update nodeThatSentThisPacket targetNode cost -> recievedNode is nodeThatSentThisPacket
//            msgPart is targetNode, msgPart2 is cost
            char *msgPart2;
            strtok_r(msgPart, " ", &msgPart2);
            int targetNode = atoi(msgPart);
            int cost = atoi(msgPart2);
//            printf("!!!%s\n",msgPart);
            cost++;//cost increased by one node

            if (routingTable[targetNode].cost > cost) {//updates routing table
                routingTable[targetNode].cost = cost;
                routingTable[targetNode].idOfNextNode = receivedNode;
                routingTable[targetNode].idOfTargetNode = targetNode;

                //creating update message from recived update message
//                createUpdateMessage(buf, wholeMessage, msgPart, cost);
//                createUpdateMessage(buf, &wholeMessage, targetNode, cost);

//                //!!
//                strcpy(wholeMessage, updateMessage);
//                char str[15];
//                char str2[15];
//                char strmsgPart[15];
//                sprintf(str, " %d", localId);
//                strcat(wholeMessage, str);
//                sprintf(strmsgPart, " %d", targetNode);
//                strcat(wholeMessage, strmsgPart);
//                sprintf(str2, " %d", cost);
//                strcat(wholeMessage, str2);
//                sprintf(buf, wholeMessage);
//                //!!

                printf("Update message from line 184: %s\n", buf);
                int a;
                for (a = 0; a < LENGHTOFARRAY; a++) {
                    strcpy(wholeMessage, updateMessage);
                    char str[15];
                    char str2[15];
                    char strmsgPart[15];
                    sprintf(str, " %d", localId);
                    strcat(wholeMessage, str);
                    sprintf(strmsgPart, " %d", routingTable[a].idOfTargetNode);
                    strcat(wholeMessage, strmsgPart);
                    sprintf(str2, " %d", routingTable[a].cost);
                    strcat(wholeMessage, str2);
                    sprintf(buf, wholeMessage);

                    int ii;
                    for (ii = 0; ii < connectionCount; ii++) {
                        if (sendto(s, buf, BUFLEN, 0, &sis[ii], slen) == -1) {
                            diep("sendto()");
                        }

                        verbPrintf("Send packet to %s:%d\nData: %s\n\n", inet_ntoa(sis[ii].sin_addr),
                                   ntohs(sis[ii].sin_port), buf);
                    }


                }



            } else if (cost >= MAXCOST) {

//                createUpdateMessage(buf,wholeMessage,targetNode,MAXCOST);
                strcpy(wholeMessage, updateMessage);
                char str[15];
                char str2[15];
                char strmsgPart[15];
                sprintf(str, " %d", localId);
                strcat(wholeMessage, str);
                sprintf(strmsgPart, " %d", targetNode);
                strcat(wholeMessage, strmsgPart);
                sprintf(str2, " %d", MAXCOST);
                strcat(wholeMessage, str2);
                sprintf(buf, wholeMessage);
                //!!
                bool alreadyKnown = false;
                int k;
                for(k = 0; k<LENGHTOFARRAY; k++){
                    if(routingTable[k].idOfTargetNode == targetNode){
                        if(routingTable[k].cost == MAXCOST){
                            alreadyKnown = true;
                        }
                        routingTable[k].cost = MAXCOST;
                        break;
                    }
                }

                if(!alreadyKnown){
                    int ii;
                    printf("Update message from line 294(MAXCOST): %s\n", buf);
                    for (ii = 0; ii < connectionCount; ii++) {
                        //if (connectionsAvailable[ii]) {

                        if (sendto(s, buf, BUFLEN, 0, &sis[ii], slen) == -1) {
                            diep("sendto()");
                        }

                        verbPrintf("Send packet to %s:%d\nData: %s\n\n", inet_ntoa(sis[ii].sin_addr),
                                   ntohs(sis[ii].sin_port), buf);
                        //    connectionsAvailable[ii] = false;
                        //}
                    }
                }




            }


        } else {
            //received message

            //printf("whole message: %s\n", wholeMessage);
            //printf("nodeNum: %s\n", nodeNum);
            //printf("msgPart: %s\n", msgPart);

            if (receivedNode == localId) {
                printf("Received packet from %s:%d\nMessage: %s\n\n",
                       inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), msgPart);
            } else {
                //send buf (the whole message) to proper node

                int j;
                for (j = 0; j < LENGHTOFARRAY; j++) {
                    if (routingTable[j].idOfTargetNode == receivedNode) {
                        int nextNode = routingTable[j].idOfNextNode;
                        int k;
                        for (k = 0; k < connectionCount; k++) {
                            if (connections[k].id == nextNode) {
                                si_other.sin_port = htons(connections[k].port);
                                if (inet_aton(connections[k].ip_address, &si_other.sin_addr) == 0) {
                                    fprintf(stderr, "inet_aton() failed 2\n");
                                    exit(1);
                                }
                                break;
                            }
                        }
                        break;
                    }
                }

                if (sendto(s, buf, BUFLEN, 0, &si_other, slen) == -1) {
                    diep("sendto()");
                }

            }
        }

        //printf("Received packet from %s:%d\nData: %s\n\n",
        //       inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

        ++i;
    }
    close(s);
    return NULL;
}

//void createUpdateMessage(char *buf, char *wholeMessage, char *msgPart, int cost) {//buggy not sure why
//    strcpy(wholeMessage, updateMessage);
//    char str[15];
//    char str2[15];
//    sprintf(str, " %d", localId);
//    strcat(wholeMessage, str);
//    strcat(wholeMessage, " ");
//    strcat(wholeMessage, msgPart);
//    strcat(wholeMessage, " ");
//    sprintf(str2, " %d", cost);
//    strcat(wholeMessage, str2);
//    sprintf(buf, wholeMessage);
//}

void createUpdateMessage(char *buf, char *wholeMessage, int msgPart, int cost) {
    strcpy(wholeMessage, updateMessage);
    char str[15];
    char str2[15];
    char strmsgPart[15];
    sprintf(str, " %d", localId);
    strcat(wholeMessage, str);
    sprintf(str2, " %d", msgPart);
    strcat(wholeMessage, strmsgPart);
    sprintf(str2, " %d", cost);
    strcat(wholeMessage, str2);
    sprintf(buf, wholeMessage);
}

//converts char array to int
int toInt(char *a) {
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

    bool readFromFile = true;

    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\nUsage: %s <ID> [ <cfg-file-name> ]\n", argv[0]);
        return -1;
    }
    if (argc > 3) {
        if (strcmp(argv[3], quiet) == 0) {
            verbosity = false;
        }

    } else if (argc > 2) {
        if (strcmp(argv[2], quiet) == 0) {
            verbosity = false;
            readFromFile = false;

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
    int result = parseRouteConfiguration(((argc > 2) && readFromFile) ? argv[2] : NULL, localId, &localPort, &connectionCount,
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

    int ii;
    for (ii = 0; ii < connectionCount; ii++) {
        memset((char *) &sis[ii], 0, sizeof(sis[ii]));
        sis[ii].sin_family = AF_INET;
        sis[ii].sin_port = htons(connections[ii].port);
        if (inet_aton(connections[ii].ip_address, &sis[ii].sin_addr) == 0) {
            fprintf(stderr, "inet_aton() failed 1\n");
            exit(1);
        }
    }

    for (ii = 0; ii < LENGHTOFARRAY; ii++) {
        connectionsAvailable[ii] = false;
        routingTable[ii].cost = MAXCOST;
    }

    pthread_create(&pthreadClient, NULL, clientThread, "foo");

    pthread_t pthreadServer;

    pthread_create(&pthreadServer, NULL, serverThread, "foo");

    printf("main is ready to run...\n");
    printf("main waiting for thread to terminate...\n");

    printf("Ready to send messages.\n");

    //accepting messages from stdin

    bool sendMessage;

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

            sendMessage = false;

            //sending message to desired port
            //printf("Message to send: %s\n", inputMsg);

            //separate parts of the message
            char nodeNum[128];// also msgPart2
            char *msgPart3;
            char wholeMessage[128];
            strcpy(wholeMessage, sendingMessage);
            strcat(wholeMessage, " ");
            strcat(wholeMessage, inputMsg);
            strcpy(nodeNum, inputMsg);
            strtok_r(nodeNum, " ", &msgPart3);
            //printf("Part 1: %s ; Part 2: %s \n", nodeNum, msgPart3);
            //printf("Receiving node is going to be: %s\n.", nodeNum);
            //choose converter or atoi ??
            //receivingNode = toInt(nodeNum);
            receivingNode = atoi(nodeNum);
            //printf("Receiving node is going to be (converted): %d\n.", receivingNode);

            si_other.sin_family = AF_INET;

            //find the port

//            printf("COnnection count: %d",connectionCount);
            for (j = 0; j < LENGHTOFARRAY; j++) {
                if (routingTable[j].idOfTargetNode == receivingNode) {
                    int nextNode = routingTable[j].idOfNextNode;
                    if (routingTable[j].cost != MAXCOST) {
                        sendMessage = true;
                    } else {
                        sendMessage = false;
                    }
                    printf("NEXT NODE: %d", nextNode);
                    int k;
                    for (k = 0; k < connectionCount; k++) {
                        if (connections[k].id == nextNode) {
                            si_other.sin_port = htons(connections[k].port);
                            if (inet_aton(connections[k].ip_address, &si_other.sin_addr) == 0) {
                                fprintf(stderr, "inet_aton() failed 2\n");
                                exit(1);
                            }
                            break;
                        }
                    }
                    break;
                }
            }
//                if (connections[j].id == receivingNode) {
//                    si_other.sin_port = htons(connections[j].port);
//                    if (inet_aton(connections[j].ip_address, &si_other.sin_addr) == 0) {
//                        fprintf(stderr, "inet_aton() failed 2\n");
//                        exit(1);
//                    }
//                    break;
//                }


//            sprintf(buf, "%s\n", inputMsg);
            sprintf(buf, "%s\n", wholeMessage);
            if (sendMessage) {
                if (sendto(s, buf, BUFLEN, 0, &si_other, slen) == -1)
                    diep("sendto()");
                printf("Send message to %s:%d\nData: %s\n\n",
                       inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);
            } else {
                printf("Message cannot be send!\n");
                printf(wholeMessage);
                int l;
                for(l = 0; l < 10; l++){
                    printf("target node: %d ", routingTable[l].idOfTargetNode);
                    printf("next node: %d ", routingTable[l].idOfNextNode);
                    printf("cost: %d\n", routingTable[l].cost);
                }
            }

        }
    }

    pthread_join(pthreadClient, NULL);
    exit(0);
}