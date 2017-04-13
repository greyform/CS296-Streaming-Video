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


typedef struct servent servent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;

int broadcastPermission = 1;
int reuse = 1;
char * localhost = "127.0.0.1";
unsigned short default_port = 9999;

static void fillAddr(unsigned short port, sockaddr_in * addr) {

    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);
    memset(&(addr->sin_zero), 0, 8);
}

int createSocket(int type, int protocol){
     int socket_desc = socket(PF_INET, type, protocol);
     if(socket_desc <0){
        perror("socket()");
        exit(1);
    }
    return socket_desc;
}

void setLocalPort(int socket_desc, unsigned short port, sockaddr_in * localAddr, int localAddr_size) { //server

    if(bind(socket_desc, localAddr, localAddr_size) != 0){
        perror("bind()");
        exit(1);
    }
}



void usage(char * command){
    fprintf(stderr, "Usage: %s <Port>\n", command);
}




int main(int argc, char *argv[]){
    if(argc != 2){
        usage(argv[0]);
        exit(1);
    }

    sockaddr_in local_addr, clientAddr;
    int localAddr_size, clientAddr_size;
    unsigned short local_port = atoi(argv[1]);


    int sockfd= createSocket(SOCK_STREAM, 0); 

    //setBroadcast
    setsockopt(socket_desc, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission));
    //set reuse
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    fillAddr(local_port, &local_addr);
    localAddr_size = sizeof(local_addr);

    setLocalPort(sockfd, local_port, &local_addr, localAddr_size);

    if (listen(sockfd, 5) < 0) /* allow 5 requests to queue up */ 
        error("ERROR on listen");

    /* Main Loop */
    clientAddr_size = sizeof(clientAddr);
    char bufUDP[BUFSIZE];


    cvNamedWindow("Webcam from Server", CV_WINDOW_AUTOSIZE);
//while(1){
    /* 
     * accept: wait for a connection request 
     */
    childfd = accept(sockfd, (sockaddr *) &clientAddr, &clientAddr_size);
    if (childfd < 0) 
      error("ERROR on accept");
    else
      printf("Connection Established with %s\n", clientAddr->sin_addr.s_addr);

    /* 
     * OpenCV: Grabbing a frame
     */

    FrameCapture = cvCaptureFromCAM(0); 
    frameORG = cvQueryFrame(FrameCapture);  
    small = cvCreateImage(cvSize(frameORG->width, frameORG->height), frameORG->depth, 3);


    if(!FrameCapture) {
        perror("Could not iniciate webcam");
        exit(1);
    }

    while((cvWaitKey(40) & 0xFF) != ESC_KEY)
    {
            small = cvQueryFrame(FrameCapture);                                     
            for(times = 0; times < MAX_PAQS; times++)
            {
                memset(bufUDP, 0, BUFSIZE);
                memcpy(bufUDP, small->imageData + BUFSIZE*times, BUFSIZE);
                if (sendto(sockUDP, bufUDP, UDPMAX, 0, (struct sockaddr *)&servUDP, length) < 0)
                {
                    perror("Failed to Send") // check errno for the problem or conection lost
                }
            }
        cvShowImage("Webcam from Server", small); // feedback on server webcam
    }
//}

    close(childfd);

}