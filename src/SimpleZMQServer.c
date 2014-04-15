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
#include <fcntl.h>

#define MAXLOGLENGTH 1023
#define MAX_CONNECTIONS 10
#define ZMQ_PORT 4242
#define SOCKET_PORT 4343

int ZMQServer(void* responder) {
	char buffer [MAXLOGLENGTH+1];
	int rc = zmq_recv (responder, buffer, MAXLOGLENGTH, ZMQ_DONTWAIT);
	if (rc  < 0) {
		if (errno  == EAGAIN)
			return 0;
		printf("ERROR: Error reading ZMQ message! %s\n", strerror(errno));
		return 0;
	}
	buffer[rc] = '\0';
	printf ("%s", buffer);
	zmq_send (responder, "Acknowledged", 12, 0);	// Simple ZMQ REQ REP pattern requires strict alternation between send & receive.
    return 1;
}

int SocketServer(int connfd) {
    static char buffer[MAXLOGLENGTH+1], *line = NULL;
    static ssize_t n = 0, last_nl = 0, rv;
    int linesPrinted = 0;

	// keep appending to end of current buffer which might contain incomplete lines.
	while((rv = read(connfd, buffer + n, MAXLOGLENGTH - n)) > 0) {
		int c;
		n += rv;
		c = n;
		buffer[n] = '\0';
		//Find the last New Line character so that we can shift possible incomplete lines to beginning.
		for ( ; c > 0; c--)
			if (buffer[c] == '\n') {
				last_nl = c;
				break;
			}
		line = strtok(buffer, "\n");
		printf("%s\n", line);
		//Print all the complete lines from current buffer.
		while ((line = strtok(NULL, "\n")) != NULL) {
			if (line - buffer < last_nl) {
				printf("%s\n", line);
				linesPrinted++;
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
	// if read returned -1 its mostly due to EAGAIN since there is no data, but a 0 means client closed connection.
	if (rv == 0)
		return -1;
    return linesPrinted;
}

int main(void) {

	unsigned ptime, ctime;

	//Declare ZMQ related variables.
	void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    char uri[15] = "tcp://*:";
    int rc;

    //Declare Socket related variables
    int listenfd = -1,connfd = -1, flags, on = 1;
    struct sockaddr_in serv_addr;

    //Initialize Socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on));
    flags = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SOCKET_PORT);
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, MAX_CONNECTIONS);


    //Initialize ZMQ socket.
    sprintf(uri + strlen(uri), "%d", ZMQ_PORT);
    rc = zmq_bind (responder, uri);
    assert (rc == 0);

    ptime = time(NULL);
    while (1) {
    	// Do a non blocking receive message on ZMQ socket.
    	int rvz = ZMQServer(responder);
    	if (rvz) {
    		ptime = time(NULL);
    	}

    	// Check if the there is no existing active listening connection.
    	if (connfd < 0) {
    		// if not accept new connection if any and make the accepted socket non blocking.
    		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
    		if (connfd >=0) {
    		    flags = fcntl(connfd, F_GETFL, 0);
    			fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
    		}
    	}
    	// if we have an active incoming connection receive available messages on the socket.
		if (connfd >= 0) {
			rvz = SocketServer(connfd);
			// if the client closed the connection then close the socket
			// 		and reset connfd so that new connectoin can be accepted.
			if (rvz == -1) {
				close(connfd);
				connfd = -1;
			}
		}
    	if (rvz) {
    		ptime = time(NULL);
    	}

    	ctime = time(NULL);
    	if (ctime - ptime >= 10) {
    		printf("No message received in the last 10 second\n");
    		ptime = ctime;
    	}
	}

    zmq_close(responder);
    zmq_ctx_destroy(context);
    close(listenfd);
    return EXIT_SUCCESS;
}
