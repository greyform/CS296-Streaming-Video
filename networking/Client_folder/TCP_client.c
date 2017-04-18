
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

ssize_t get_message_size(int socket) {
    ssize_t size;
    ssize_t read_bytes = read(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

void read_from_server(int sockfd, IplImage** img_ptr, sockaddr_in* server_address, socklen_t length) {
    // Silence the unused parameter warning.    
    ssize_t retval = 1;
    char *buffer = NULL;
    ssize_t recvMsgSize = 0;


    retval = get_message_size(sockfd);
    if (retval > 0) {
        buffer = calloc(1, retval);
        recvMsgSize = recvfrom(sockfd, buffer, retval, 0, 
                            (sockaddr *)server_address, &length);
            //read_all_from_socket(serverSocket, buffer, retval);
    }
    if (retval == recvMsgSize)
            memcpy((*img_ptr)->imageData, buffer, retval);
    free(buffer);
    buffer = NULL;
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
    char* buffer = NULL; // Buffer for echo string
    int recvMsgSize; // Size of received message
    int totalsize = 0;

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
        
        //for(times = 0; times < MAX_PAQS; times++)
        //{ 
            totalsize = 0;
            buffer = malloc(sizeof(int));
            //recvMsgSize = recvfrom(sockfd, buffer, sizeof(int), 0, (struct sockaddr *)&server_address, &length);

            //if(recvMsgSize <0)
            //{
               // perror("Failed to Read");
                //break;
            //}
            int size = *((int*)buffer);
            fprintf(stderr, "%d, %d\n", recvMsgSize, size);
            free(buffer);
            size = 2764800;
            buffer = calloc(1, 2764800+1);

            while (totalsize < size){
                fprintf(stderr, "%d, %d\n", size, totalsize);
                recvMsgSize = recvfrom(sockfd, buffer, size-totalsize, 0, (struct sockaddr *)&server_address, &length);
                if(recvMsgSize <0)
                {
                    perror("Failed to Read");
                    break;
                }
                totalsize+= recvMsgSize;
            }
            
            fprintf(stderr, "%d, %d\n", size, totalsize);

           
            memcpy(img->imageData, buffer, size);
            free(buffer);
            buffer = NULL;
        //}

        //read_from_server(sockfd, &img, &server_addr, length);

        cvShowImage("TCP Streaming from Server", img);
        memset(img->imageData, 0, 640*480*3);
    }

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);


    return 0;
}






/*#include <sys/socket.h> // For socket(), connect(), send(), and recv()
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
#define BUFSIZE 65500
#define MAX_PAQS (640*480*3/65500)
#define MESSAGE_SIZE_DIGITS 8
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;

int reuse = 1;


static void fillAddr(const char * hostname, unsigned short port, sockaddr_in * addr) {
    //memset(&addr, 0, sizeof(addr));
    addr->sin_family = AF_INET;


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

ssize_t get_message_size(int socket) {
    ssize_t size;
    ssize_t read_bytes = read(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

void read_from_server(int sockfd, IplImage** img_ptr, sockaddr_in* server_address, socklen_t length) {
    // Silence the unused parameter warning.    
    ssize_t retval = 1;
    char *buffer = NULL;
    ssize_t recvMsgSize = 0;


    retval = get_message_size(sockfd);
    if (retval > 0) {
        buffer = calloc(1, retval);
        recvMsgSize = recvfrom(sockfd, buffer, retval, 0, 
                            (sockaddr *)server_address, &length);
            //read_all_from_socket(serverSocket, buffer, retval);
    }
    if (retval == recvMsgSize)
            memcpy((*img_ptr)->imageData, buffer, retval);
    free(buffer);
    buffer = NULL;
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

    fillAddr(server_address, server_port,  &server_addr);
    length = sizeof(server_addr);
    fprintf(stderr, "%d\n", __LINE__);

    int times;
    char* buffer = NULL; // Buffer for echo string
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

        cvShowImage("TCP Streaming from Client", img);
        memset(img->imageData, 0, 640*480*3);
    }


    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);


    return 0;
}*/