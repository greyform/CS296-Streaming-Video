#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <webcam.c>

/*struct sockaddr_in {
    short            sin_family;   
    unsigned short   sin_port;     
    struct in_addr   sin_addr;     
    char             sin_zero[8];  
};*/

/*struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};*/

// Function to fill in address structure given an address and port
static void fillAddr(const string &address, unsigned short port, 
                     sockaddr_in &addr) {
  memset(&addr, 0, sizeof(addr));  // Zero out address structure
  addr.sin_family = AF_INET;       // Internet address

  hostent *host;  // Resolve name
  if ((host = gethostbyname(address.c_str())) == NULL) {
    fprintf(stderr, "Failed to resolve host name!\n");
  }
  addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
  addr.sin_port = htons(port);     // Assign port in network byte order
}

  /*
      Send the given buffer as a UDP datagram to the
      specified address/port
*/
void sendTo(const void *buffer, int bufferLen, const string &foreignAddress, unsigned short foreignPort) {
  sockaddr_in destAddr;
  fillAddr(foreignAddress, foreignPort, destAddr);
  // Write out the whole buffer as a single message.
  if (sendto(sockDesc, (char *) buffer, bufferLen, 0,
             (sockaddr *) &destAddr, sizeof(destAddr)) != bufferLen) {
    fprintf(stderr, "Failed to send\n");
  }
}

int recvFrom(void *buffer, int bufferLen, string &sourceAddress,
    unsigned short &sourcePort){
  sockaddr_in clntAddr;
  socklen_t addrLen = sizeof(clntAddr);
  int rtn;
  if ((rtn = recvfrom(sockDesc, (char *) buffer, bufferLen, 0, 
                      (sockaddr *) &clntAddr, (socklen_t *) &addrLen)) < 0) {
    fprintf(stderr, "Failed to receive\n");
  }
  sourceAddress = inet_ntoa(clntAddr.sin_addr);
  sourcePort = ntohs(clntAddr.sin_port);

  return rtn;
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        fprintf(stderr, "Too free arguments\n");
        return 1;
    } 

    int sockfd = 0, n = 0;
    struct sockaddr_in serv_addr; 

    char recvBuff[1024];
    memset(recvBuff, '0', 1024);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  //will change to SOCK_DGRAM, IPPROTO_UDP for UDP connection.
    {
        printf("\n Failed to create socket \n");
        return 1;
    } 


    //serv_addr.sin_family = AF_INET;
    //serv_addr.sin_port = htons(5000); 
    //memset(&(serv_addr.sin_zero), '0', 8); 



    fprintf(stderr, "%d\n", __LINE__);
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Failed to connect\n");
       return 1;
    } 
	fprintf(stderr, "%d\n", __LINE__);


    while ( (n = recv(sockfd, recvBuff, sizeof(recvBuff)-1, 0)) > 0)
    {
        recvBuff[n] = 0;
	       printf("%d\n", __LINE__);
        if(fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Fputs error\n");
        }
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 

    return 0;
}
