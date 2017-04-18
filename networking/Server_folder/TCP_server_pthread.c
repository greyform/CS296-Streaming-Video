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

typedef struct servent servent;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;

int broadcastPermission = 1;
int reuse = 1;


#define MAX_CLIENTS 8
#define MESSAGE_SIZE_DIGITS 8

void *process_client(void *p);

static volatile int endSession;
static volatile int serverSocket;

static volatile int clientsCount;
static volatile int clients[MAX_CLIENTS];

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*typedef struct wrap{
    IplImage * small;
    intptr_t id;

}; wrapper*/
//CvCapture* FrameCapture;
//IplImage* frameORG;
//IplImage* small;


/**
 * Cleanup function called in main after `run_server` exits.
 * Server ending clean up (such as shutting down clients) should be handled
 * here.
 */
void cleanup() {
    for(int i = 0; i <clientsCount; i++){
        if(clients[i] != -1) shutdown(clients[i], SHUT_RDWR);
        close(clients[i]);
    }
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
    pthread_exit(NULL);
}



intptr_t check_available(int childfd){
    for(intptr_t i = 0; i < MAX_CLIENTS; i++){
        if(clients[i] == -1){
            clients[i] = childfd;
            return i;
        }       
    }
    return -1;
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

/**
 * Sets up a server connection.
 * Does not accept more than MAX_CLIENTS connections.  If more than MAX_CLIENTS
 * clients attempts to connects, simply shuts down
 * the new client and continues accepting.
 * Per client, a thread should be created and 'process_client' should handle
 * that client.
 * Makes use of 'endSession', 'clientsCount', 'client', and 'mutex'.
 *
 * port - port server will run on.
 *
 * If any networking call fails, the appropriate error is printed and the
 * function calls exit(1):
 *    - fprtinf to stderr for getaddrinfo
 *    - perror() for any other call
 */
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

    if (bind(serverSocket, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind()");
        exit(1);
    }
    freeaddrinfo(result);

    if (listen(serverSocket, MAX_CLIENTS) != 0) {
        perror("listen()");
        exit(1);
    }
    //struct sockaddr_in *result_addr = (struct sockaddr_in *) result->ai_addr;
    for(int i = 0; i < MAX_CLIENTS; i++){
        clients[i] = -1;       
    }

  int childfd;


    /* 
     * OpenCV: Grabbing a frame
     */



    while(1){
        pthread_mutex_lock(&mutex);
        if(endSession){ 
            pthread_mutex_unlock(&mutex);
            return;
        }
        if(clientsCount >= MAX_CLIENTS){
            pthread_mutex_unlock(&mutex); 
            continue;
        }
        pthread_mutex_unlock(&mutex);

        childfd = accept(serverSocket, NULL, NULL);

        pthread_mutex_lock(&mutex);
        intptr_t id = check_available(childfd);
        clientsCount++;
        printf("Currently servering %d clients\n", clientsCount);
        pthread_mutex_unlock(&mutex);

        pthread_t pool;
        if(pthread_create(&pool, NULL, process_client, (void*) id)){
            perror("pthread_create()");
            exit(1);
        }

    }

}

/**
 * Broadcasts the message to all connected clients.
 *
 * message  - the message to send to all clients.
 * size     - length in bytes of message to send.
 */
void write_to_clients(const char *message, size_t size) {  //message = img->ImageData, size= img->ImageSize
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            ssize_t retval = write_message_size(size, clients[i]);
            if (retval > 0) {
                retval = write_all_to_socket(clients[i], message, size);
            }
            if (retval == -1) {
                perror("write(): ");
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

/**
 * Handles the reading to and writing from clients.
 *
 * p  - (void*)intptr_t index where clients[(intptr_t)p] is the file descriptor
 * for this client.
 *
 * Return value not used.
 */
void *process_client(void *p) {
    pthread_detach(pthread_self());
    intptr_t clientId = (intptr_t)p;
    ssize_t retval = 1;
    char *buffer = NULL;
    printf("Serving client %d .\n", (int)clientId);

    int times;
    char bufUDP[BUFSIZE];


    CvCapture* FrameCapture = cvCaptureFromCAM(0); 
    IplImage* frameORG = cvQueryFrame(FrameCapture);  
    IplImage* small = cvCreateImage(cvSize(frameORG->width, frameORG->height), frameORG->depth, 3);
    if(!FrameCapture) {
        perror("Could not iniciate webcam");
        exit(1);
    }




    while((cvWaitKey(40) & 0xFF) != 'x') {
        small = cvQueryFrame(FrameCapture);
        fprintf(stderr, "here");
        for(times = 0; times < MAX_PAQS; times++){
            memset(bufUDP, 0, BUFSIZE);
            memcpy(bufUDP, small->imageData+BUFSIZE*times, BUFSIZE);
            if(sendto(clients[clientId], bufUDP, BUFSIZE, 0, NULL, 0) < 0){
                perror("failed to send()");
            }

            
        }


        //write_to_clients(small->imageData, small->imageSize);
        cvShowImage("Webcam from Server", small);
    }

    
    close(clients[clientId]);

    pthread_mutex_lock(&mutex);
    clients[clientId] = -1;
    clientsCount--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "./server <port>\n");
        return -1;
    }

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    signal(SIGINT, close_server);
    run_server(argv[1]);
    cleanup();
    pthread_exit(NULL);
}