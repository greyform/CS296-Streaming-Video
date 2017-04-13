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
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <ml.h>
#include <cxcore.h>
#include "vector.h"

#define FRAME_HEIGHT 720
#define FRAME_WIDTH 1280
#define FRAME_INTERVAL (1000/30)
#define PACK_SIZE 4096 //udp pack size; note that OSX limits < 8100 bytes
#define ENCODE_QUALITY 80
  #define BUFSIZE 1024
#define MAX_PAQS (640*480*3/1024)
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;

int reuse = 1;


static void fillAddr(const char * hostname, unsigned short port, sockaddr_in * addr) {
    //memset(&addr, 0, sizeof(addr));
    addr->sin_family = AF_INET;

    /* gethostbyname: get the server's DNS entry */
    hostent *host; 
    if((host = gethostbyname(hostname)) == NULL){  // ??
        herror("fillAddr::gethostbyname()");
        exit(2);
    }
    addr->sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
    addr->sin_port = htons(port);
    memset(&(addr->sin_zero), 0, 8);

}


int createSocket(int type, int protocol){
     int socket_desc = socket(AF_INET, type, protocol);
     if(socket_desc <0){
        perror("socket()");
        exit(1);
    }
    return socket_desc;
}


void usage(char * command){
    fprintf(stderr, "Usage: %s <Server> <Server Port>\n", command);
}


int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        usage(argv[0]);
        exit(1);
    } 
    sockaddr_in server_addr;
    socklen_t length;

    char * server_address = argv[1];
    unsigned short server_port = atoi(argv[2]);

    int sockfd= createSocket(SOCK_STREAM, 0); 
    //set reuse
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    fprintf(stderr, "%d\n", __LINE__);
    /*Server's addr_in */
    fillAddr(server_address, server_port,  &server_addr);
    length = sizeof(server_addr);
    fprintf(stderr, "%d\n", __LINE__);

    int times;
    char buffer[BUFSIZE]; // Buffer for echo string
    int recvMsgSize; // Size of received message

    fprintf(stderr, "%d\n", __LINE__);
    if( connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
       perror("Connect()");
       exit(2);
    } 
    else{
        printf("Connection Established with %s\n", server_address);
    }
    fprintf(stderr, "%d\n", __LINE__);

    cvNamedWindow("TCP Streaming from Server", CV_WINDOW_AUTOSIZE);
    IplImage* img = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);

    while((cvWaitKey(40) & 0xFF) != 'x')
    {
        for(times = 0; times < MAX_PAQS; times++)
        {
            if ((recvMsgSize = recvfrom(sockfd, buffer, BUFSIZE, 0, 
                            (struct sockaddr *)&server_address, &length)) < 0)
            {
                perror("Failed to Read");
                break;
            }
            memcpy(img->imageData + BUFSIZE*times, buffer, BUFSIZE);
        }

        cvShowImage("TCP Streaming from Server", img);
    }

    close(sockfd);


    return 0;
}