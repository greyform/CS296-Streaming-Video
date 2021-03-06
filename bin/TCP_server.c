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
#include <time.h>




/* The following headers can vary depending on 
* how library is set up on the computer */
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cvaux.h>
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/cvconfig.h"
#include "opencv2/core/core_c.h"
#include "opencv2/videoio/videoio_c.h"
#include "opencv2/imgcodecs/imgcodecs_c.h"
#include <ml.h>
#include <cxcore.h>


#define FRAME_INTERVAL (1000/30)
#define ENCODE_QUALITY 50
#define MESSAGE_SIZE_DIGITS 8
#define MAX_CLIENTS 10

int camera;
int ResizeFactor = 1;


void *process_client(void *p);

static volatile int endSession;
static volatile int serverSocket;

static volatile int clientsCount;
static volatile int clients[MAX_CLIENTS];

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int width;
int height;
int depth;

void* accept_connection(void*id);

void cleanup() {
    for(int i = 0; i <clientsCount; i++){
        if(clients[i] != -1) shutdown(clients[i], SHUT_RDWR);
        close(clients[i]);
    }
    close(serverSocket); 
}

/**
 * Signal handler for SIGINT.
 * Used to set flag to end server.
 */
void close_server() {
    endSession = 1;
    shutdown(serverSocket, SHUT_RDWR); 
    cleanup();
    exit(1);
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

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
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


/**
 * Handles the reading to and writing from clients.
 *
 * p  - (void*)intptr_t index where clients[(intptr_t)p] is the file descriptor
 * for this client.
 *
 * Return value not used.
 */
void* process_client(void* p) {

    CvMat *mat;
    int params[] = {CV_IMWRITE_JPEG_QUALITY, ENCODE_QUALITY};
    CvCapture* FrameCapture; 
    CvVideoWriter * writer;
    IplImage* frameORG, *small, *jpg;
    int senMsgSize;
    char key;
    int Switch = 0;
    float* data;
    int step;
    CvSize size;
    int x, y;


    if ((FrameCapture = cvCreateCameraCapture(camera)) == NULL) {
        fprintf(stderr, "Could not initiate webcam.\n");
        return NULL;
    }
    
    small = cvCreateImage(cvSize(width, height), depth, 3); 

    while(1){
        /* Grab a frame from the webcam and resize it into the appropriate scale */
        frameORG = cvQueryFrame(FrameCapture);
        cvResize(frameORG, small, CV_INTER_LINEAR);

        /* Encode the image to compressed jpeg form */
        mat = cvEncodeImage(".jpg", small, params);
        cvGetRawData(mat, (uchar**)&data, &step, &size);
   
        key = cvWaitKey(30) & 0XFF;

        /* if user pressed 'x', stop sending data */
        if(key == 'x'){ break;}

        /* if user pressed 's', start/stop saving the frames into an avi video file */
        if(key == 's'){
            Switch = !Switch;
            if(Switch){
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                char filename[128];
                sprintf(filename, "camera_%d_%d_%d_%d:%d.avi", tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
                int frameW = (int)cvGetCaptureProperty(FrameCapture , CV_CAP_PROP_FRAME_WIDTH);
                int frameH = (int)cvGetCaptureProperty(FrameCapture , CV_CAP_PROP_FRAME_HEIGHT);
                writer = cvCreateVideoWriter(filename, CV_FOURCC('M','J','P','G'), 
                    CV_CAP_PROP_FPS, cvSize(frameW, frameH), 1);
            }
        }
        if( Switch ){
            cvWriteFrame(writer, small);
        }

        /* Send the frames in a for-loop to all connected clients. Thread-safe */
        for(int i = 0; i < MAX_CLIENTS; i++){
            pthread_mutex_lock(&mutex);
                if(clients[i]!=-1){
                    int childfd = clients[i];
                    pthread_mutex_unlock(&mutex);
                    senMsgSize = //send(childfd, small->imageData, small->imageSize);
                                write_all_to_socket(childfd, (char*)&step, 4);
                    if(senMsgSize <0 ){
                        perror("failed to send");
                        //close_connection();
                    }
                    else fprintf(stderr, "About to send %d bytes of data\n", step);                         
                    senMsgSize = //send(childfd, small->imageData, small->imageSize);
                                //write_all_to_socket(childfd, small->imageData, small->imageSize);
                                write_all_to_socket(childfd, (char*)data, step);
                    if(senMsgSize <0 ){
                        perror("failed to send");
                        //close_connection();
                    }   
                    else fprintf(stderr, "%d\n", senMsgSize);
                }
                else
                    pthread_mutex_unlock(&mutex);
        }

        //jpg = cvDecodeImage(mat, CV_LOAD_IMAGE_COLOR);
        cvShowImage("Local Webcam", small); // feedback on server webcam
        
    }
    cvReleaseImage(&frameORG);
    cvReleaseImage(&small);
    cvReleaseMat(&mat);
    cvReleaseCapture(&FrameCapture);
    cvDestroyWindow("Webcam from Server");
    cvReleaseVideoWriter(&writer);
    return NULL;
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
        close(serverSocket);
        exit(1);
    }
    else
        fprintf(stderr, "getaddrinfo: SUCCESS\n");

    if (bind(serverSocket, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind()");
        close(serverSocket);
        exit(1);
    }
    else
        fprintf(stderr, "bind: SUCCESS\n");
    freeaddrinfo(result);

    if (listen(serverSocket, MAX_CLIENTS) != 0) {
        perror("listen()");
        close(serverSocket);
        exit(1);
    }
    else
        fprintf(stderr, "Listen: SUCCESS\n");
    
    
    for(int i = 0; i < MAX_CLIENTS; i++){
        clients[i] = -1;       
    }

    /* 
     * OpenCV: Grabbing a frame
     */

    CvCapture* FrameCapture = cvCaptureFromCAM(camera); 
    IplImage* frameORG = cvQueryFrame(FrameCapture);  
    int window = cvNamedWindow("Webcam from Server", CV_WINDOW_AUTOSIZE);
    int params[] = {CV_IMWRITE_JPEG_QUALITY, 50};
    width = frameORG->width/ResizeFactor;
    height = frameORG->height/ResizeFactor;
    depth = frameORG->depth;


    /*Create a thread to handle connections*/
    pthread_t pool;
    if(pthread_create(&pool, NULL, accept_connection, NULL)){
        perror("pthread_create()");
        close(serverSocket);
        exit(1);
    }
    process_client(NULL);
}

void* accept_connection(void*id){
    int childfd;
    int senMsgSize;

     while(1){
        pthread_mutex_lock(&mutex);
        if(endSession){ 
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        if(clientsCount >= MAX_CLIENTS){
            pthread_mutex_unlock(&mutex); 
            continue;
        }
        pthread_mutex_unlock(&mutex);

        childfd = accept(serverSocket, NULL, NULL);
        fprintf(stderr, "Accept(): SUCCESS\n");

        /* Send image dimension to each newly connected client */
        senMsgSize = write_message_size(width,childfd);
        senMsgSize = write_message_size(height, childfd);
        senMsgSize = write_message_size(depth, childfd);

        /* Store cliendfd in global array */
        pthread_mutex_lock(&mutex);
        intptr_t id = check_available(childfd);
        clientsCount++;
        pthread_mutex_unlock(&mutex);

    } 
    return NULL;
}


void usage(char * command){
    fprintf(stderr, "Usage: %s <Port> [camera] [resize_factor]\n", command);
}

int main(int argc, char *argv[]){
    if(argc != 2 && argc != 3 && argc != 4){
        usage(argv[0]);
        exit(1);
    }
    if(argv[2]){
        camera = atoi(argv[2]);
        fprintf(stderr, "%d\n", camera);
    }
    if(argv[2] && argv[3]){
        ResizeFactor = atoi(argv[3]);
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
    return 0;
}
