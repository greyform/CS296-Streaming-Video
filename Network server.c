#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 


int main(int argc, char *argv[])
{
    int connfd = 0;
    struct sockaddr_in serv_addr; 

    serv_addr.sin_family = AF_INET;
    //serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); // dest port
    serv_addr.sin_addr.s_addr = INADDR_ANY;  //convert dest ip from dots/number string to struct in_addr
    memset(&(serv_addr.sin_zero), '0', 8);


    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc <0){
        printf("\n Failed to create socket \n");
        return 1;
    }
    if(bind(socket_desc, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0){
        printf("\n Failed to bind \n");
        return 1;
    }

    //connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

    char sendBuff[1025];
    time_t ticks; 
    memset(sendBuff, '0', sizeof(sendBuff)); 

    listen(socket_desc, 10); //backlog is the number of connections allowed on the incoming queue.

    while(1)
    {
        connfd = accept(socket_desc, (struct sockaddr*)NULL, NULL); 
        ticks = time(NULL);
        fprintf(stderr, "%s\n", ctime(&ticks));
        snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
        write(connfd, sendBuff, strlen(sendBuff)); 

        close(connfd);
        sleep(1);
     }
}
