
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
#include <signal.h>


/* The following headers can vary depending on 
* how library is set up on the computer */
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "opencv2/imgcodecs/imgcodecs_c.h"
#include <ml.h>
#include <cxcore.h>



#define FRAME_INTERVAL (1000/30)
#define ENCODE_QUALITY 50
#define MESSAGE_SIZE_DIGITS 8

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;

int reuse = 1;
int sockfd;


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

void close_program(){
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    exit(1);
}

int connect_to_server(const char *host, const char *port) {
     if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
     {
         printf("\n Failed to create socket \n");
         return 1;
     }
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    setsockopt(sockfd , SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

     struct addrinfo hints, *result;
     memset(&hints, 0, sizeof(hints));
     hints.ai_family = AF_INET;
     hints.ai_socktype = SOCK_STREAM;

     int s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
            close(sockfd);
            exit(1);
    }
    if(connect(sockfd, result->ai_addr, result->ai_addrlen) == -1){
        perror("connect");
        close(sockfd);
        exit(1);
    }
    freeaddrinfo(result);
    return sockfd;
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

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_program;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }
    signal(SIGINT, close_program);


    sockaddr_in server_addr;
    socklen_t length;
    sockfd = connect_to_server(argv[1], argv[2]);

    int retval = 1;
    int times;
    int recvMsgSize; // Size of received message
    int totalsize = 0;


    /* Reading data about the dimension of received Image*/
    int width = get_message_size(sockfd);
    if(width <= 0){
      perror("wrong message size");
      close_program();
      exit(1);
    }
    else
      fprintf(stderr, "width: %d\n", width);

    int height = get_message_size(sockfd);
    if(height <= 0){
      perror("wrong message size");
      close_program();
      exit(1);
    }
    else
      fprintf(stderr, "height: %d\n", height);

    int depth = get_message_size(sockfd);
    if(depth <= 0){
      perror("wrong message size");
      close_program();
      exit(1);
    }
    else
      fprintf(stderr, "depth: %d\n", depth);

    /*Create image based on dimensions received */

    IplImage* img = cvCreateImage(cvSize(width, height), depth, 3);
    int imageSize = img->imageSize;

    CvMat *mat; 
    int step;

    /* Receiving data until x is pressed */
    while((cvWaitKey(40) & 0xFF) != 'x')
    {   
        recvMsgSize = read_all_from_socket(sockfd, (char*)&step, 4);
        if(recvMsgSize <= 0){
            perror("wrong message size");
            close_program();
            exit(1);
        }
        else 
            fprintf(stderr, "About to received %d bytes of data\n", step);
        mat = cvCreateMat(1, step, CV_8UC1);
        float data[step];
        totalsize = read_all_from_socket(sockfd, (char*)data, step);
        if(totalsize <= 0){
            perror("wrong message size");
            close_program();
            exit(1);
        }
        else 
            fprintf(stderr, "Received %d bytes of data\n", totalsize);

        cvSetData(mat, (void *)data, step);
        img = cvDecodeImage(mat, CV_LOAD_IMAGE_COLOR);
        /* display image */
        cvShowImage("TCP Streaming from Server", img);
    }

    cvReleaseImage(&img);
    cvReleaseMat(&mat);
    cvDestroyWindow("TCP Streaming from Server");
    close_program();
    return 0;
}