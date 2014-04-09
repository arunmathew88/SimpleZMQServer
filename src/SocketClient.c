/*
 * SocketClient.c
 *
 *  Created on: 09-Apr-2014
 *      Author: arunmathew
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXLOGLENGTH 1023
#define SOCKET_PORT 4343

int main (void)
{
    FILE *log = fopen("../data/syslog", "r");
    char buffer[MAXLOGLENGTH];
    int sockfd = 0;
    unsigned n = 0;
    char recv_buff[1024];
    struct sockaddr_in serv_addr;

    if (log == NULL) {
    	printf("ERROR: log file opening failed!\n");
    	return 1;
    }

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SOCKET_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
       printf("\n Error : Connect Failed \n");
       return 1;
    }

    do {
    	char *str = fgets(buffer, MAXLOGLENGTH, log);
    	if (str == NULL) continue;
    	while ((n += write(sockfd, buffer + n, strlen(buffer) - n)) < strlen(buffer)) ;
    	n = 0;
    	printf("Sent: %s", buffer);
    } while (!feof(log));
    close(sockfd);
    return 0;
}
