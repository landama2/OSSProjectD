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

bool verbosity = false;
void verbPrintf(const char *message) {

    if (verbosity)
        printf(message);
    else {

    }
    // Do nothing
}




int main(int argc, char *argv[]) {
    if (argc == 1) /* argc should be 2 for correct execution */{
        /* We print argv[0] assuming it is the program name */
        verbPrintf("ID of this node must be specified!");
//        printf( "usage: %s filename", argv[0] );
    } else if (argc == 2) {
        printf("usage: %s filename\n", argv[0]);
        printf("Number of this node is: %s", argv[1]);
    } else {
        struct sockaddr_in si_me, si_other;
        return 0;
        // We assume argv[1] is a filename to open
        FILE *file = fopen(argv[1], "r");

        /* fopen returns 0, the NULL pointer, on failure */
        if (file == 0) {
            printf("Could not open file\n");
        } else {
            int x;
            /* read one character at a time from file, stopping at EOF, which
               indicates the end of the file.  Note that the idiom of "assign
               to a variable, check the value" used below works because
               the assignment statement evaluates to the value assigned. */
            while ((x = fgetc(file)) != EOF) {
                printf("%c", x);
            }
            fclose(file);
        }
    }
    // //
    printf("Test commit2");
    printf("Misa tady byla a upravila si mail a jeste si ho dala private!!");
    return 0;
}