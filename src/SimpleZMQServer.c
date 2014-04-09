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
#include <pthread.h>

#define MAXLOGLENGTH 1023

long counter = 0;

void* ZMQServer(void *td) {
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
        counter++;
        buffer[rc] = '\0';
        printf ("%s", buffer);
        zmq_send (responder, "Acknowledged", 12, 0);
    }
    return NULL;
}

int main(void) {
	pthread_t thread[2];
	pthread_create(&thread[0], NULL, ZMQServer, NULL);
	while (1) {
		printf("Counter: %ld\n", counter);
		sleep(10);
	}
	pthread_join(thread[0], NULL);
	return EXIT_SUCCESS;
}
