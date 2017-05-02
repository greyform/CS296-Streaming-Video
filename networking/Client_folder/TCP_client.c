
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
#define MESSAGE_SIZE_DIGITS 8
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;

int reuse = 1;
int sockfd;


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

void close_program(){
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
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
            exit(1);
    }
fprintf(stderr, "before connect\n");
    if(connect(sockfd, result->ai_addr, result->ai_addrlen) == -1){
        perror("connect");
        exit(1);
    }
fprintf(stderr, "after connect\n");
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
    sockaddr_in server_addr;
    socklen_t length;
    fprintf(stderr, "before connect\n");
    sockfd = connect_to_server(argv[1], argv[2]);
    fprintf(stderr, "after connect\n");

    int retval = 1;
    int times;
    int recvMsgSize; // Size of received message
    int totalsize = 0;


    cvNamedWindow("Client", CV_WINDOW_AUTOSIZE);
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

    IplImage* img = cvCreateImage(cvSize(width, height), depth, 3);
    int imageSize = img->imageSize;
    fprintf(stderr, "%d\n", imageSize);

    while((cvWaitKey(40) & 0xFF) != 'x')
    {
        totalsize = read_all_from_socket(sockfd, img->imageData, imageSize);
        if(totalsize <= 0){
            perror("wrong message size");
            close_program();
            exit(1);
        }
        else 
            fprintf(stderr, "Received %d bytes of data\n", totalsize);
  
        cvShowImage("TCP Streaming from Server", img);
    }

    close_program();

    return 0;
}