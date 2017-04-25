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
//#include <pthread.h>
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
#define MAX_CLIENTS 8

void process_client();

int serverSocket;
int childfd;
int endSession;
//static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct servent servent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;


void cleanup() {
    //for(int i = 0; i <clientsCount; i++){
      //  if(clients[i] != -1) shutdown(clients[i], SHUT_RDWR);
        //close(clients[i]);
    //}
    close(childfd);
    close(serverSocket); 
    // Your code here.
}

/**
 * Signal handler for SIGINT.
 * Used to set flag to end server.
 */
void close_server() {
    endSession = 1;
    // add any additional flags here you want.
    shutdown(serverSocket, SHUT_RDWR); 
    cleanup();
    exit(1);
    //pthread_exit(NULL);
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
  ssize_t bytes_read; 
  ssize_t total_read = 0;
  while(1)
  {
    bytes_read = read(socket, buffer + total_read, count - total_read);
    if (bytes_read > 0) total_read += bytes_read;

    if (bytes_read == 0) return 0; 
    else if (bytes_read == -1 && errno == EINTR) continue;
    else if (bytes_read == -1) return -1;  
    else if ((size_t) total_read == count) return total_read;  
  }
  return 0; 
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
ssize_t write_message_size(size_t size, int socket) {
    uint32_t s = htonl((uint32_t) size); 
    ssize_t write_bytes = write_all_to_socket(socket, (char *)&s, MESSAGE_SIZE_DIGITS);

    return write_bytes;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}


void usage(char * command){
    fprintf(stderr, "Usage: %s <Port>\n", command);
}

/**
 * Handles the reading to and writing from clients.
 *
 * p  - (void*)intptr_t index where clients[(intptr_t)p] is the file descriptor
 * for this client.
 *
 * Return value not used.
 */
void process_client() {
    //pthread_detach(pthread_self());
    //intptr_t clientId = (intptr_t)p;
    ssize_t retval = 1;
    //char *buffer = NULL;
   // printf("Serving client %d .\n", (int)clientId);

    /* 
     * OpenCV: Grabbing a frame
     */
    int senMsgSize;

    CvCapture* FrameCapture = cvCaptureFromCAM(0); 
    IplImage* frameORG = cvQueryFrame(FrameCapture);  
    IplImage* small = cvCreateImage(cvSize(frameORG->width, frameORG->height), frameORG->depth, 3);

    if(!FrameCapture) {
        perror("Could not iniciate webcam");
        exit(1);
    }

    senMsgSize = write_message_size(frameORG->width,childfd);
    senMsgSize = write_message_size(frameORG->height, childfd);
    senMsgSize = write_message_size(frameORG->depth, childfd);



    while((cvWaitKey(40) & 0xFF) != 'x')
    //for(int i =0; i<1000; i++)
    {
            small = cvQueryFrame(FrameCapture);
            fprintf(stderr, "small_size: %d\n", small->imageSize);     
            senMsgSize = write_message_size(small->imageSize,childfd);  
            if(senMsgSize <0 ){
                perror("failed to send");
                exit(1);
            }
            else
                fprintf(stderr, "write_msg_size: %d.\n", senMsgSize);                              
            senMsgSize = //send(childfd, small->imageData, small->imageSize);
                        write_all_to_socket(childfd, small->imageData, small->imageSize);
            if(senMsgSize <0 ){
                perror("failed to send");
                exit(1);
            }
            else
                fprintf(stderr, "write_img: %d.\n", senMsgSize);      
        cvShowImage("Webcam from Server", small); // feedback on server webcam
      
    }

    close(childfd);
}




void run_server(char *port) {
    
    int s;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL, port, &hints, &result);

    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }
    else
        fprintf(stderr, "getaddrinfo: SUCCESS\n");

    if (bind(serverSocket, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind()");
        exit(1);
    }
    else
        fprintf(stderr, "bind: SUCCESS\n");
    freeaddrinfo(result);

    if (listen(serverSocket, MAX_CLIENTS) != 0) {
        perror("listen()");
        exit(1);
    }
    else
        fprintf(stderr, "Listen: SUCCESS\n");


        childfd = accept(serverSocket, NULL, NULL);
        fprintf(stderr, "Accept(): SUCCESS\n");
        process_client();
}


int main(int argc, char *argv[]){
    if(argc != 2){
        usage(argv[0]);
        exit(1);
    }
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

   /* CvCapture* FrameCapture = cvCaptureFromCAM(0); 
    IplImage* frameORG = cvQueryFrame(FrameCapture);  
    IplImage* small = cvCreateImage(cvSize(frameORG->width, frameORG->height), frameORG->depth, 3);
    IplImage* img = cvCreateImage(cvSize(frameORG->width, frameORG->height), frameORG->depth, 3);
    

     while((cvWaitKey(40) & 0xFF) != 'x')
    //for(int i =0; i<1000; i++)
    {
        small = cvQueryFrame(FrameCapture); 
        cvShowImage("Webcam from Server", small); // feedback on server webcam
        memcpy(img->imageData, small->imageData, small->imageSize);
        cvShowImage("NewImage", img);
      
    }*/


    signal(SIGINT, close_server);
    run_server(argv[1]);

    cleanup();
    //pthread_exit(NULL);
    return 0;


}



