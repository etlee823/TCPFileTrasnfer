#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/syscall.h>

#define MAX_BUF 1024
#define PORT 6665

void handleSigInt(int);
void cleanUp();
int myListenSocket, clientSocket;
FILE *file;

int main()
{
    
    char logName[MAX_BUF], ip[MAX_BUF], buffer[MAX_BUF], str[MAX_BUF];
    int  port, i, addrSize, bytesRcv;
    struct sockaddr_in  myAddr, clientAddr;
    socklen_t len;
    
    port = PORT;

    printf("--== Server Running --==\n");
    
    printf("--== Creating Socket --==");
    myListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (myListenSocket < 0) {
        printf("Error: Server couldn't open socket\n");
        exit(-1);
    }
    
    
    printf("--== Setting up Server Address --==\n");
    memset(&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = htons(port);
    
    
    printf("--== Server Binding to socket --==\n");
    i = bind(myListenSocket, (struct sockaddr *) &myAddr, sizeof(myAddr));
    if (i < 0) {
        printf("Error: Server couldn't bind socket\n");
        exit(-1);
    }
    
    
    printf("--== Server Listening to Socket --==\n");
    i = listen(myListenSocket, 5);
    if (i < 0) {
        printf("Error: Server couldn't listen\n");
        exit(-1);
    }
    
    
    printf("--== Server waiting for connection request --==\n");
    addrSize = sizeof(clientAddr);
    clientSocket = accept(myListenSocket, (struct sockaddr *) &clientAddr,  &addrSize);
    if (clientSocket < 0) {
        printf("Error: Server couldn't accept the connection\n");
        exit(-1);
    }
    
    printf("--== Connection Successful, Server Ready to read Messages! --==\n");
    
    while (1){
        bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (strcmp(buffer, "quit") == 0) {
                cleanUp();
        }
        if (strcmp(buffer, "ls\0") == 0) {
            int rc;
            //rc = syscall(SYS_ls, );
            //send(clientSocket, buffer, sizeof(buffer), 0);
        }
        memset(buffer, 0, sizeof(buffer));
    }
    

    
    
    cleanUp();
    
    return 0;
}

/*         Name: cleanUp
 *  Description: closes the listening socket, client socket, and the log file
 *   Parameters: none
 *       Return: void
 */
void cleanUp(){
    /* Closing sockets and log file */
    close(myListenSocket);
    close(clientSocket);
    
    printf("clean up finished, terminating process");
    exit(0);
}