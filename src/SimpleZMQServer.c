/*
 ============================================================================
 Name        : SimpleZMQServer.c
 Author      : Arun Mathew
 Version     :
 Copyright   : 
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <zmq.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define MAXLOGLENGTH 1023

int main(void) {
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    int rc = zmq_bind (responder, "tcp://*:4242");
    assert (rc == 0);
    while (1) {
        char buffer [MAXLOGLENGTH+1];
        int rc = zmq_recv (responder, buffer, MAXLOGLENGTH, 0);
        if (rc  < 0) {
        	printf("ERROR: Error reading ZMQ message!\n");
        	continue;
        }
        buffer[rc] = '\0';
        printf ("%s", buffer);
        zmq_send (responder, "Acknowledged", 12, 0);
    }
	return EXIT_SUCCESS;
}
