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
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXLOGLENGTH 1023
#define MAX_CONNECTIONS 10
#define ZMQ_PORT 4242
#define SOCKET_PORT 4343

long counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* ZMQServer(void *td) {
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    char uri[15] = "tcp://*:";
    int rc;
    sprintf(uri + strlen(uri), "%d", ZMQ_PORT);
    rc = zmq_bind (responder, uri);
    assert (rc == 0);
    while (1) {
        char buffer [MAXLOGLENGTH+1];
        int rc = zmq_recv (responder, buffer, MAXLOGLENGTH, 0);
        if (rc  < 0) {
        	printf("ERROR: Error reading ZMQ message!\n");
        	continue;
        }
        pthread_mutex_lock( &mutex );
        counter++;
        pthread_mutex_unlock( &mutex );

        buffer[rc] = '\0';
        printf ("%s", buffer);
        zmq_send (responder, "Acknowledged", 12, 0);	// Simple ZMQ REQ REP pattern requires strict alternation between send & receive.
    }
    return NULL;
}

void* SocketServer(void *td) {
    int listenfd = 0,connfd = 0;
    unsigned n = 0, last_nl = 0;
    char buffer[MAXLOGLENGTH+1], *line = NULL;
    struct sockaddr_in serv_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SOCKET_PORT);
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, MAX_CONNECTIONS);

    while (1) {
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

		// n+= .... keep appending to end of current buffer which might contain incomplete lines.
		while((n += read(connfd, buffer + n, MAXLOGLENGTH - n)) > 0) {
			int c = n;
			buffer[n] = '\0';
			//Find the last New Line character so that we can shift possible incomplete lines to beginning.
			for ( ; c > 0; c--)
				if (buffer[c] == '\n') {
					last_nl = c;
					break;
				}
			line = strtok(buffer, "\n");
			printf("%s\n", line);
	        pthread_mutex_lock( &mutex );
	        counter++;
	        pthread_mutex_unlock( &mutex );
	        //Print all the complete lines from current buffer.
			while ((line = strtok(NULL, "\n")) != NULL) {
				if (line - buffer < last_nl) {
					printf("%s\n", line);
			        pthread_mutex_lock( &mutex );
			        counter++;
			        pthread_mutex_unlock( &mutex );
				} else {
					break;
				}
			}
			//For shifting the incomplete line to beginning of the buffer.
			for (c=0, last_nl++; last_nl < n; c++, last_nl++) {
				buffer[c] = buffer[last_nl];
			}
			n = c;
		}
    }
    return NULL;
}

int main(void) {
	pthread_t thread[2];
	pthread_create(&thread[0], NULL, ZMQServer, NULL);
	pthread_create(&thread[1], NULL, SocketServer, NULL);
	while (1) {
		long old_counter;
		sleep(10);
        pthread_mutex_lock( &mutex );
        old_counter = counter;			//Copying the value so that the lock is for minimal period.
        counter = 0;
        pthread_mutex_unlock( &mutex );
        if (old_counter == 0)
        	printf("No message received in the last 10 second\n");
	}
	pthread_join(thread[1], NULL);
	pthread_join(thread[0], NULL);
	return EXIT_SUCCESS;
}
