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
#include <time.h>

#define FRAME_HEIGHT 720
#define FRAME_WIDTH 1280
#define FRAME_INTERVAL (1000/30)
#define PACK_SIZE 4096 //udp pack size; note that OSX limits < 8100 bytes
#define ENCODE_QUALITY 80
  #define BUFSIZE 1024
#define MAX_PAQS (640*480*3/1024)

#define MESSAGE_SIZE_DIGITS 8
#define MAX_CLIENTS 5

int camera;

pid_t pid[MAX_CLIENTS];
pid_t end[MAX_CLIENTS];

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
    // add any additional flags here you want.
    shutdown(serverSocket, SHUT_RDWR); 
    cleanup();
    exit(1);
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
    fprintf(stderr, "Usage: %s <Port> [camera]\n", command);
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
    //pthread_detach(pthread_self());
    intptr_t clientId = (intptr_t)p;
    ssize_t retval = 1;
    char *buffer = NULL;
    printf("Serving client %d .\n", (int)clientId);
    /* 
     * OpenCV: Grabbing a frame
     */
    int senMsgSize;
    pthread_mutex_lock(&mutex);
    int childfd = clients[clientId];
    pthread_mutex_unlock(&mutex);

    CvCapture* FrameCapture = cvCaptureFromCAM(camera); 
   // int window = cvNamedWindow("Webcam from Server", CV_WINDOW_NORMAL);
    IplImage* frameORG = cvQueryFrame(FrameCapture);  
    IplImage* small = cvCreateImage(cvSize(frameORG->width, frameORG->height), frameORG->depth, 3);

    if(!FrameCapture) {
        perror("Could not iniciate webcam");
        exit(1);
    }

    senMsgSize = write_message_size(frameORG->width,childfd);
    senMsgSize = write_message_size(frameORG->height, childfd);
    senMsgSize = write_message_size(frameORG->depth, childfd);

    while(1)
    //for(int i=0; i<10000000; i++)
    {
            small = cvQueryFrame(FrameCapture);
            //cvResize(frameORG, small, CV_INTER_LINEAR );
            //fprintf(stderr, "small_size: %d\n", small->imageSize);     
            senMsgSize = write_message_size(small->imageSize,childfd);  
            if(senMsgSize <0 ){
                perror("failed to send");
                exit(1);
            }                             
            senMsgSize = //send(childfd, small->imageData, small->imageSize);
                        write_all_to_socket(childfd, small->imageData, small->imageSize);
            if(senMsgSize <0 ){
                perror("failed to send");
                exit(1);
            }     
        cvShowImage("Webcam from Server", small); // feedback on server webcam
    }

    close(childfd);
    pthread_mutex_lock(&mutex);
    clients[clientId] = -1;
    clientsCount--;
    pthread_mutex_unlock(&mutex);

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
    
    
    for(int i = 0; i < MAX_CLIENTS; i++){
        clients[i] = -1;       
    }

    /* 
     * OpenCV: Grabbing a frame
     */

    CvCapture* FrameCapture = cvCaptureFromCAM(camera); 
    IplImage* frameORG = cvQueryFrame(FrameCapture);  
   // fprintf(stderr, "%d\n", __LINE__);
    int window = cvNamedWindow("Webcam from Server", CV_WINDOW_AUTOSIZE);
    IplImage* small = cvCreateImage(cvSize(frameORG->width, frameORG->height), frameORG->depth, 3);
    //fprintf(stderr, "%d\n", __LINE__);
    width = frameORG->width;
    height = frameORG->height;
    depth = frameORG->depth;
    //fprintf(stderr, "%d\n", __LINE__);
    /*Create a thread to handle connections*/
    pthread_t pool;
    if(pthread_create(&pool, NULL, accept_connection, NULL)){
        perror("pthread_create()");
        exit(1);
    }

    
    int senMsgSize;
    //pthread_mutex_lock(&mutex);
   // int childfd = clients[clientId];
   // pthread_mutex_unlock(&mutex);

    if(!FrameCapture) {
        perror("Could not iniciate webcam");
        exit(1);
    }

char key;
int Switch = 0;

 CvVideoWriter * writer;

    while( 1){
        

        if(key == 'x'){ break;}
        small = cvQueryFrame(FrameCapture);
       // cvResize(frameORG, small, CV_INTER_LINEAR );
        //int status;
        key = cvWaitKey(30) & 0XFF;
        if(key == 's'){
            Switch = !Switch;
            if(Switch){
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                char filename[128];
                sprintf(filename, "camera_%d_%d_%d_%d:%d.avi", tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
                int frameW = (int)cvGetCaptureProperty(FrameCapture , CV_CAP_PROP_FRAME_WIDTH);
                int frameH = (int)cvGetCaptureProperty(FrameCapture , CV_CAP_PROP_FRAME_HEIGHT);
                writer = cvCreateVideoWriter(filename, CV_FOURCC('M','J','P','G'), //CV_CAP_PROP_FOURCC, CV_FOURCC('M','J', 'P', 'G'),
                    CV_CAP_PROP_FPS, cvSize(frameW, frameH), 1);
            }

            
        }
        if( Switch ){
            cvWriteFrame(writer, small);
        }

        for(int i = 0; i < MAX_CLIENTS; i++){
            //pid[i] = fork();
            pthread_mutex_lock(&mutex);
            //fprintf(stderr, "%d\n", __LINE__);
            /*if(pid[i] == -1){
                perror("fork error");
                exit(EXIT_FAILURE);
            }
            else if(pid[i] == 0){
            //child            
            //fprintf(stderr, "%d\n", __LINE__);*/
                if(clients[i]!=-1){
                    int childfd = clients[i];
                    pthread_mutex_unlock(&mutex);
                    senMsgSize = write_message_size(small->imageSize,childfd); 
                    if(senMsgSize <0 ){
                        perror("failed to send");
                        exit(1);
                    }                             
                    senMsgSize = //send(childfd, small->imageData, small->imageSize);
                                write_all_to_socket(childfd, small->imageData, small->imageSize);
                    if(senMsgSize <0 ){
                        perror("failed to send");
                        exit(1);
                    }   
                }
                else
                    //exit(EXIT_SUCCESS);
                    pthread_mutex_unlock(&mutex);
            //}
        }//for loop end

        //parent
        /*for(int i = 0; i < clientsCount; i++){
            end[i] = waitpid(pid[i], &status, WNOHANG|WUNTRACED);
            if(end[i] == -1){
                perror("waitpid error");
                exit(EXIT_FAILURE); 
            }
            else if( end[i] == pid[i]){
             if(WIFSIGNALED(status))
                    fprintf(stderr, "Child ended because of an uncaught signal.\n");
                else if(WIFSTOPPED(status))
                    fprintf(stderr, "Child process has stopped.\n");
                    
            }
        }*/

        cvShowImage("Webcam from Server", small); // feedback on server webcam
        
    }
        cvReleaseImage(&frameORG);
        cvReleaseImage(&small);
        cvReleaseCapture(&FrameCapture);
        cvDestroyWindow("Webcam from Server");
        cvReleaseVideoWriter(&writer);
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
        senMsgSize = write_message_size(width,childfd);
        senMsgSize = write_message_size(height, childfd);
        senMsgSize = write_message_size(depth, childfd);

        pthread_mutex_lock(&mutex);
        intptr_t id = check_available(childfd);
        clientsCount++;
        printf("Currently servering %d clients\n", clientsCount);
        pthread_mutex_unlock(&mutex);

    } 
    return NULL;
}



int main(int argc, char *argv[]){
    if(argc != 2 && argc != 3){
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
    //fprintf(stderr, "%d\n", __LINE__);
    if(argv[2])
        camera = atoi(argv[2]);
    run_server(argv[1]);
    //fprintf(stderr, "%d\n", __LINE__);

    cleanup();
    pthread_exit(NULL);
    return 0;


}



