#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
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
void handlels(char*);
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
        
        printf("waiting for new input \n");
        
        bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0);
        buffer[bytesRcv] = '\0';
        
        if (strcmp(buffer, "quit") == 0) {
            
            printf("received quit command\n");
            cleanUp();
            
        } else if (buffer[0] == 'l' && buffer[1] == 's') {
            
            handlels(buffer);
            send(clientSocket, buffer, sizeof(buffer), 0);

            //memset(buffer, 0, sizeof(buffer));
            
        } else if (buffer[0] == 'c' && buffer[1] == 'd') {
            printf("received cd command\n");

            char directory[MAX_BUF] = {'\0'};
            int i;
            
            for (i = 3; buffer[i] != '\0'; i++){
                directory[i-3] = buffer[i];
            }
            
            if (chdir( directory) == 0 ){
                printf("cd success\n");
                send(clientSocket, "success\0", sizeof("success\0"), 0);
            } else {
                printf("cd fail\n");
                send(clientSocket, "fail\0", sizeof("fail\0"), 0);
            }
            
        } else if (buffer[0] == 'g' && buffer[1] == 'e' && buffer[2] == 't'){
            printf("received get command \n");
            
        } else if ( buffer[0] == 'p' && buffer[1] == 'u' && buffer[2] == 't'){
            printf("received put command \n");
            
        } else if ( buffer[0] == 'm' && buffer[1] == 'k' && buffer[2] == 'd'&& buffer[3] == 'i'&& buffer[4] == 'r') {
            printf("received mkdir command \n");
            
            char directory[MAX_BUF];
            int i;
            
            for (i = 6; buffer[i] != '\0'; i++){
                directory[i-6] = buffer[i];
            }
            printf("%s \n", buffer);
            printf("%s \n", directory);
            
            /** making directory */
            if (mkdir(directory, 0700) == 0) {
                printf("mkdir success\n");
                send(clientSocket, "success", sizeof(buffer), 0);
            } else {
                printf("mkdir fail\n");
                send(clientSocket, "fail", sizeof(buffer), 0);
            }
        } else {
            printf("getting this string\n %s\n", buffer);
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

/*         Name: ls
 *  Description: fills buffer with output of ls command
 *   Parameters: char array buffer
 *       Return: void
 */
void handlels(char* buffer){
    printf("received ls command\n");
    file = popen("ls", "r");
    char c;
    int k = 0;
    while ((c = fgetc(file)) != EOF) {
        buffer[k] = c;
        k++;
    }
    buffer[k] = 0;
    fflush(file);
    pclose(file);
}
