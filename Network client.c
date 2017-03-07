#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

/*struct sockaddr_in {
    short            sin_family;   
    unsigned short   sin_port;     
    struct in_addr   sin_addr;     
    char             sin_zero[8];  
};*/

/*struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};*/


int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        fprintf(stderr, "Too free arguments");
        return 1;
    } 

    int sockfd = 0, n = 0;
    struct sockaddr_in serv_addr; 

    char recvBuff[1024];
    memset(recvBuff, '0', 1024);
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 
    memset(&serv_addr, '0', sizeof(serv_addr)); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\nFailed to interpret IP address\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Failed to connect\n");
       return 1;
    } 

    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        if(fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Fputs error\n");
        }
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 

    return 0;
}