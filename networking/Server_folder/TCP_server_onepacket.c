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
#include <pthread.h>
#include <signal.h>
#include <ml.h>
#include <cxcore.h>
#include "vector.h"

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cvaux.h>
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/cvconfig.h"
#include "opencv2/core/core_c.h"
#include "opencv2/videoio/videoio_c.h"

#define FRAME_HEIGHT 720
#define FRAME_WIDTH 1280
#define FRAME_INTERVAL (1000/30)
#define PACK_SIZE 4096 //udp pack size; note that OSX limits < 8100 bytes
#define ENCODE_QUALITY 80
  #define BUFSIZE 1024
#define MAX_PAQS (640*480*3/1024)
#define MESSAGE_SIZE_DIGITS 8

typedef struct servent servent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;

int broadcastPermission = 1;
int reuse = 1;





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

    if(bind(socket_desc, (sockaddr *)localAddr, localAddr_size) != 0){
        perror("bind()");
        exit(1);
    }
}



ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
  ssize_t bytes_written; 
  ssize_t total_written = 0; 
  while (1)
  {
    bytes_written = write(socket, buffer + total_written, count - total_written);
    if (bytes_written > 0) total_written += bytes_written;

    if (bytes_written == 0) return 0; 
    else if (bytes_written == -1 && errno == EINTR) continue;
    else if (bytes_written == -1) return -1;
    else if ( (size_t)total_written == count) return total_written;  
  }
  return 0; 
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(ssize_t size, int socket) {
    ssize_t s = htonl(size); 
    ssize_t write_bytes = write_all_to_socket(socket, (char *)&s, MESSAGE_SIZE_DIGITS);

    return write_bytes;
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
    socklen_t localAddr_size, clientAddr_size;
    unsigned short local_port = atoi(argv[1]);


    int sockfd= createSocket(SOCK_STREAM, 0); 
    int childfd;

    //setBroadcast
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission));
    //set reuse
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    fillAddr(local_port, &local_addr);
    localAddr_size = sizeof(local_addr);

    setLocalPort(sockfd, local_port, &local_addr, localAddr_size);

    if (listen(sockfd, 5) < 0) /* allow 5 requests to queue up */ 
        perror("listen()");

    /* Main Loop */
    clientAddr_size = sizeof(clientAddr);
    int times;
    char * bufUDP = NULL;
    ssize_t senMsgSize=0;


    cvNamedWindow("Webcam from Server", CV_WINDOW_AUTOSIZE);




//while(1){
    /* 
     * accept: wait for a connection request 
     */
    childfd = accept(sockfd, (sockaddr *) &clientAddr, &clientAddr_size);
    if (childfd < 0) 
      perror("accept()");
    else
      printf("Connection Established with %u\n", clientAddr.sin_addr.s_addr);

    /* 
     * OpenCV: Grabbing a frame
     */

    CvCapture* FrameCapture = cvCaptureFromCAM(0); 
    IplImage* frameORG = cvQueryFrame(FrameCapture);  
    IplImage* small = cvCreateImage(cvSize(frameORG->width, frameORG->height), frameORG->depth, 3);


    if(!FrameCapture) {
        perror("Could not iniciate webcam");
        exit(1);
    }

    while((cvWaitKey(40) & 0xFF) != 'x')
    {
            small = cvQueryFrame(FrameCapture);                                     
            //for(times = 0; times < MAX_PAQS; times++)
            //{
            bufUDP = calloc(1, small->imageSize+1);
            fprintf(stderr, "%d\n", small->imageSize);
            memcpy(bufUDP, small->imageData, small->imageSize);
                //senMsgSize = sendto(childfd, (char*)&(small->imageSize), 8, 0, (struct sockaddr *)&clientAddr, clientAddr_size);
            
            senMsgSize = sendto(childfd, (char*)&(small->imageSize), 8, 0, 
                                            (struct sockaddr *)&clientAddr, clientAddr_size);
            fprintf(stderr, "%d, %d\n", senMsgSize, small->imageSize)
                if(senMsgSize < 0){
                    perror("Failed to Send"); 
                }

            senMsgSize = sendto(childfd, bufUDP, small->imageSize, 0, (struct sockaddr *)&clientAddr, clientAddr_size);

                if (senMsgSize < 0)
                {
                    perror("Failed to Send"); // check errno for the problem or conection lost
                }
            
        cvShowImage("Webcam from Server", small); // feedback on server webcam
    }


//}
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    close(childfd);

}
