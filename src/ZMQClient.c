/*
 * ZMQClient.c
 *
 *  Created on: 09-Apr-2014
 *      Author: arunmathew
 */


#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define MAXLOGLENGTH 1023

int main (void)
{
    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    FILE *log = fopen("../data/syslog", "r");
    char buffer[MAXLOGLENGTH];

    zmq_connect (requester, "tcp://localhost:4242");

    if (log == NULL) {
    	printf("ERROR: log file opening failed!\n");
    	return 1;
    }
    do {
    	char *str = fgets(buffer, MAXLOGLENGTH, log);
    	if (str == NULL) continue;
    	zmq_send (requester, buffer, strlen(buffer), 0);
    	printf("Sent: %s", buffer);
    	zmq_recv (requester, buffer, 12, 0);
    	buffer[12]='\0';
    	printf(" ------%s\n", buffer);
    } while (!feof(log));
    zmq_close (requester);
    zmq_ctx_destroy (context);
    return 0;
}
