
#include <sys/types.h>

#include <sys/socket.h> // For socket(), connect(), send(), and recv()
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h> // For inet_addr()
#include <netdb.h>  //For gethostbyname()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //for close()
#include <errno.h>  //for errno
#include <string.h>  //for memset
#include <sys/types.h>  //data types  

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#include <cv.h>
//#include <highgui.h>
#include <ml.h>
#include <cxcore.h>
#include "vector.h"

#include <opencv/cv.h>
#include <opencv/highgui.h>


/*struct sockaddr_in {
    short            sin_family;   
    unsigned short   sin_port;     
    struct in_addr   sin_addr;     
    char             sin_zero[8];  
};*/

/*struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};*/
typedef struct servent servent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;

int broadcastPermission = 1;
int reuse = 1;

/*unsigned short resolveService(char * service, char* protocol){
    servent * serv;
    if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL) {
        return atoi(service.c_str());
    }
    else
        return ntohs(serv->s_port);
}*/
static void fillAddr(const char * address, unsigned short port, sockaddr_in * addr) {
    
    //fprintf(stderr, "%d: %p\n", __LINE__, addr);
    addr->sin_family = AF_INET;
   // fprintf(stderr, "%d\n", __LINE__);
    hostent *host; 
    if((host = gethostbyname(address)) == NULL){  // ??
        herror("fillAddr::gethostbyname()");
        exit(2);
    }
    //fprintf(stderr, "%d\n", __LINE__);
    addr->sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
    //fprintf(stderr, "%d\n", __LINE__);
    addr->sin_port = htons(port);
}

int createSocket(int type, int protocol){
     int socket_desc = socket(PF_INET, type, protocol);
     if(socket_desc <0){
        perror("socket()");
        exit(1);
    }
    return socket_desc;
}

void cleanup(int socket_desc){
    close(socket_desc);
}


void sendTo(int socket_desc, const void *buffer, int bufferLen, const char *foreignAddress, unsigned short foreignPort) {
    sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    fprintf(stderr, "%d: %d\n", __LINE__, bufferLen);
    fillAddr(foreignAddress, foreignPort, &destAddr);

  // Write out the whole buffer as a single message.
    if (sendto(socket_desc, buffer, bufferLen, 0,
             (sockaddr *) &destAddr, sizeof(destAddr)) != bufferLen) {
        perror("sendTo::sendto()");
        exit(1);
    }
}

int recvFrom(int socket_desc, void *buffer, int bufferLen){
    //sockaddr_in clntAddr;
    //socklen_t addrLen = sizeof(clntAddr);
    //int rtn;


   // printf("%s\n", (char*)buffer);
    int received = 0;
    sockaddr_in source;
    socklen_t addrLen = sizeof(source);

    if((received= recvfrom(socket_desc, buffer, bufferLen, 0, (sockaddr *) 
               &source, &addrLen) < 0)){
        perror("recvfrom()");
        exit(1);
    }

  return received;
}




void usage(char * command){
    fprintf(stderr, "Usage: %s <Server> <Server Port>\n", command);
}

int main(int argc, char *argv[])
{
    if (argc!=3) { // Test for correct number of arguments
        usage(argv[0]);
        exit(1);
    }

    char * server_address = argv[1];
    unsigned short server_port = atoi(argv[2]);

    int socket_desc = createSocket(SOCK_DGRAM, IPPROTO_UDP); 

    //setsockopt(socket_desc, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission));
    //setsockopt(socket_desc, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    //setLocalPort(socket_desc, port);


    //char* sendString = argv[3];               // Third arg:  string to broadcast

    char * buff = (char*)malloc(65507);

    cvNamedWindow("TCP Video Receiver", CV_WINDOW_AUTOSIZE);
    vector * videoBuffer = vector_create(NULL, NULL, NULL);

    while(1){
        //char* msg = "Hello Kugo | Miao miao miao miao miao";
        //sendTo(socket_desc, msg, strlen(msg), server_address, server_port);
        //sleep(3);
        //break;

        int result = recvFrom(socket_desc, buff, 65507);
        if(result<0){
            perror("Failed to recieve");
            continue;
        }
        printf("gotcha size: %d.\n", result);

        vector_resize(videoBuffer, result);
        memcpy((char*) vector_get(videoBuffer, 0), buff, result);

        IplImage * image = cvDecodeImage(videoBuffer, 1);
        cvShowImage("TCP Video Reciever", &image);

        cvWaitKey(5);
    }

    cleanup(socket_desc);

    return 0;
}


/*

    int sockfd = 0, n = 0;
    struct sockaddr_in serv_addr; 

    char recvBuff[1024];
    memset(recvBuff, '0', 1024);
    fprintf(stderr, "%d\n", __LINE__);
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Failed to create socket \n");
        return 1;
    } 
    //fprintf(stderr, "%d\n", __LINE__);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 
    memset(&(serv_addr.sin_zero), '0', 8); 

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

    while ( (n = recv(sockfd, recvBuff, sizeof(recvBuff)-1, 0)) > 0)
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
}*/
