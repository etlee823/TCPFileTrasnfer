#include <stdio.h>        // For printf() and fprintf().
#include <sys/socket.h>   // For socket(), connect(), send(), recv().
#include <arpa/inet.h>    // For sockaddr_in, inet_addr().
#include <stdlib.h>       // For atoi().
#include <string.h>       // For memset(), strstr().
#include <unistd.h>       // For close(), access(), exec().

#define BUFSIZE 1024    // Buffer size.
//#define DEBUG 0         // If defined, print statements will be enabled for debugging.


/////////////////////////////////////////////////////////////////////
// Function protoypes.
/////////////////////////////////////////////////////////////////////

// Exits the program with error an message
void Die(const char *message) { perror(message); exit(1); }

// Returns 0 if a string starts with a specfified substring.
int StartsWith(const char *string, const char *substring);

// Prints a message of available commands.
void HelpMessage();

// Returns 0 if the file name specifed exists in the current diretory.
int FileExists(const char *filename);

// Returns 0 if the specified file is succesfully sent to the server.
int PutFileRemote(const char *filename);

int GetFileRemote(const char *filename);

// Returns the number of substrings in a string.
int NumInputs(char *string);

char *SecondSubstring(char* string);

// Returns 0 if message is sucessfully sent.
int SendMessage(int socket, char *buffer);

// Returns the number of bytes received.
int ReceiveMessage(int socket, char *buffer);

// Handles the request for the ls command.
int HandleRequestLs(int socket, char *cmdbuffer, char *msgbuffer);

// Sends a message to the server and expects a single message back 
// from the server with the result of the command.
int HandleRequest(int socket, char *cmdbuffer, char *msgbuffer);

// Sends a message to the server alerting it is about to send a message
// and expects a message that says "Ready to accept file". The client will
// then continuously send messages of the file, and send a message with a 
// null character to let the user it is finished. Output to user that the client
// has finished sending the file to the server.
int HandleRequestPut(int socket, char *cmdbuffer, char *msgbuffer);


/////////////////////////////////////////////////////////////////////
// Main.
/////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  int sockfd;                         // Socket descripter
  struct sockaddr_in serveraddress;   // Server address
  unsigned short serverport;          // Server port
  unsigned int stringlen;             // Length of string
  char cmdbuffer[BUFSIZE];            // Buffer for command
  char msgbuffer[BUFSIZE];            // msgbuffer for string
  char *servIP;                       // Server IP address (dotted)

/*
  // for testing main logic only, skipping socket creation
  #ifdef DEBUG
  goto skip;
  #endif
*/

  // check for correct # of arguments (1 or 2)
  if ((argc < 2) || (argc > 3)) {
    fprintf(stderr, "Usage: %s <Server IP> [<Port>]\n", argv[0]);
    return -1;
  }

  // Server IP
  servIP = argv[1];

  // Check if port is specified
  if (argc == 3) {
    serverport = atoi(argv[2]);
  } else {
    serverport = 7; // 7 is a well know port for echo service
  }

  #ifdef DEBUG
  printf("[DEBUG] Creating TCP socket...\n");
  #endif

  // Create a TCP socket
  if ((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }

  #ifdef DEBUG
  printf("[DEBUG] Created a TCP socket...\n");
  #endif

  // Construct the server address structure
  memset(&serveraddress, 0, sizeof(serveraddress));
  serveraddress.sin_family      = AF_INET;
  serveraddress.sin_addr.s_addr = inet_addr(servIP);  // convert to network byte
  serveraddress.sin_port        = htons(serverport);  // convert to port

  #ifdef DEBUG
  printf("[DEBUG] Connecting to server %s...\n", servIP);
  #endif

  // Connect to the server
  if (connect(sockfd, (struct sockaddr *) &serveraddress, sizeof(serveraddress)) < 0) {
    Die("Failed to connect to server");
  }

  #ifdef DEBUG
  printf("[DEBUG] Connected to server... connect()\n");
  #endif

  /*
  #ifdef DEBUG
  skip:
  #endif
  */

  // Return value variable for functions
  int rv = 0;
  int loop = 1;
  memset(cmdbuffer, 0, sizeof(char)*BUFSIZE);
  memset(msgbuffer, 0, sizeof(char)*BUFSIZE);
  // Main logic code
  while (loop) {
    printf("ft> ");
    fgets(cmdbuffer, BUFSIZE, stdin);

      cmdbuffer[strlen(cmdbuffer)-1] = 0;
    // replace '\n' at the end of user input with '\0'.
    //char *pos;
    //if ((pos = strchr(cmdbuffer, '\n')) != NULL) {
    //  *pos = '\0';
    //}

    // NumInputs replaces the buffer used, so create a new buffer.
    char pstring[BUFSIZE];
    strcpy(pstring, cmdbuffer);

    // Check the number of inputs given by the user.
    rv = NumInputs(pstring);

    #ifdef DEBUG
    printf("[DEBUG] rv = NumInputs() = %i\n", rv);
    printf("[DEBUG] cmdbuffer = '%s'\n", cmdbuffer);
    #endif

    // This big if/else could be changed.
    // 1 command input
    if (rv == 1) {
      #ifdef DEBUG
      printf("[DEBUG] 1 word input\n");
      #endif

      if (strcmp(cmdbuffer, "quit") == 0) {
        stringlen = strlen(cmdbuffer);

        #ifdef DEBUG
        printf("--== Sending message '%s' to the server --==\n", cmdbuffer);
        #endif

        // Send the 'quit' message to the server.
        if ((rv = send(sockfd, cmdbuffer, sizeof(cmdbuffer), 0)) != stringlen) {
          Die("Failed to send 'quit' message to server");
        }

        #ifdef DEBUG
        printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
        #endif

        // Exit program.
        loop = 0;
      } else if (strcmp(cmdbuffer, "help") == 0) {
        HelpMessage();
      } else if (strcmp(cmdbuffer, "ls") == 0) {
        #ifdef DEBUG
        printf("[DEBUG] ls command\n");
        #endif

        HandleRequestLs(sockfd, cmdbuffer, msgbuffer);
      } else if (strcmp(cmdbuffer, "clear") == 0) {
        system("clear");
      } else {
        printf("Invalid command.\n");
        HelpMessage();
      } // End of 1 command input.
    } else if (rv == 2) { // 2 command inputs.
      // Basic format validation for option and 1 argument.
      if ((StartsWith(cmdbuffer, "get ") == 0) && strstr(cmdbuffer, " ")) {
        #ifdef DEBUG
        printf("[DEBUG] get <remote-file> command\n");
        #endif

      } else if ((StartsWith(cmdbuffer, "put ") == 0) && strstr(cmdbuffer, " ")) {
        #ifdef DEBUG
        printf("[DEBUG] put <file-name> command\n");
        #endif

/*
        // 1. Verify <file-name> exists

        // Need to extract the file name from cmdbuffer..
        if (FileExists("testfile") == 0) {
          #ifdef DEBUG
          printf("[DEBUG] file name: %s does exist\n", cmdbuffer);
          #endif

          // 2. Send file to server through messages
          if (PutFileRemote("<file-name>")) {
            printf("put <file-name> failed with error: %i", rv);
          } else { 
            printf("%s was successful\n", cmdbuffer);
          }
        }

        // Send 'put <file-name>' message to server.

        // Begin sending messages of the file line by line.

        // Output that the message transfer is complete.
        */
        HandleRequestPut(sockfd, cmdbuffer, msgbuffer);

      } else if ((StartsWith(cmdbuffer, "cd") == 0) && strstr(cmdbuffer, " ")) {
        #ifdef DEBUG
        printf("[DEBUG] cd <directory> command\n");
        #endif

        HandleRequest(sockfd, cmdbuffer, msgbuffer);
      } else if ((StartsWith(cmdbuffer, "mkdir") == 0) && strstr(cmdbuffer, " ")) {
        #ifdef DEBUG
        printf("[DEBUG] mkdir <directory-name> command\n");
        #endif

        HandleRequest(sockfd, cmdbuffer, msgbuffer);
      } else {
        printf("Invalid command.\n");
        HelpMessage();
      } // End of 2 command input.
    } else {
      printf("Invalid command.\n");
      HelpMessage();
    }
  } // End while loop.
} // End main.


/////////////////////////////////////////////////////////////////////
// Function implementations.
/////////////////////////////////////////////////////////////////////

int StartsWith(const char *string, const char *substring) {
  return strncmp(string, substring, strlen(substring));
}

void HelpMessage() {
  printf("Commands are:\n\n");
  printf("ls:\t\t\t\t print a listing of the contents of the current directory\n");
  printf("get <remote-file>:\t\t retrieve the <remote-file> on the server and store it in the current directory\n");
  printf("put <file-name>:\t\t put and store the file from the client machine to the server machine.\n");
  printf("cd <directory-name>:\t\t change the directory on the server\n");
  printf("mkdir <directory-name>:\t\t create a new sub-directory named <directory-name>\n");
}

int FileExists(const char *filename) {
  return access(filename, F_OK);
}

int PutFileRemote(const char *filename) {
  return 0;
}

int GetFileRemote(const char *filename) {
    return 0;
}

int NumInputs(char *string) {
  // send ls command
  #ifdef DEBUG
  printf("[DEBUG] inside NumInputs. String passed was: '%s'\n", string);
  #endif

  int numsubstrings = 0;
  char *token = strtok(string, " ");
  while (token) {
    #ifdef DEBUG
    printf("[DEBUG] token: %s\n", token);
    #endif

    numsubstrings++;
    token = strtok(NULL, " ");
  }

  return numsubstrings;
}

char *SecondSubstring(char* string) {
  char *token = strtok(string, " ");
  return strtok(NULL, " ");
}

int SendMessage(int socket, char *buffer) {
  //unsigned int stringlen = strlen(message);
  //message[stringlen] = '\0';

  #ifdef DEBUG
  printf("Inside SendMessage: '%s'\n", buffer);
  #endif

  // Send command to server
  if (send(socket, buffer, sizeof(char)*BUFSIZE, 0) < 0) {
    Die("Failed to send message");
  }

  #ifdef DEBUG
  printf("Sent message: '%s', successfully\n", buffer);
  #endif

  return 0;
}

int ReceiveMessage(int socket, char *buffer) {
  #ifdef DEBUG
  printf("Inside ReceiveMessage.\n");
  #endif

  int bytesRecvd = 0;

  // Send command to server
  if ((bytesRecvd = recv(socket, buffer, sizeof(char)*BUFSIZE, 0)) < 0) {
    Die("Failed to receive message");
  }
  buffer[bytesRecvd] = '\0';
  
  return bytesRecvd;
}

int HandleRequestLs(int socket, char *cmdbuffer, char *msgbuffer) {
  //unsigned int stringlen = strlen(cmdbuffer);
  //size_t stringlen = sizeof(char)*BUFSIZE;

  #ifdef DEBUG
  printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
  #endif

  // Send command to server.
  if (SendMessage(socket, cmdbuffer) < 0) {
    Die("Failed to send message");
  }

  #ifdef DEBUG
  printf("[DEBUG] Handling '%s' request to server\n", cmdbuffer);
  #endif

  int bytesRecvd = 0;

  // Read results from server until a null terminator is found.
  //while (1) {
    if ((bytesRecvd = ReceiveMessage(socket, msgbuffer)) < 0) {
      Die("Failed to receive message");
    }
    
    msgbuffer[bytesRecvd] = '\0';

    // Check if it is the last message.
    //if (msgbuffer[0] == '\0') {
    //  break;
    //}

    // Output the server results.
    printf("%s", msgbuffer);

    // Clear msgbuffer.
    memset(msgbuffer, 0, sizeof(char)*BUFSIZE);
  //}

  #ifdef DEBUG
  printf("--== Received entire result of '%s' from the server --==\n", cmdbuffer);
  #endif

  return 0;
}

int HandleRequest(int socket, char *cmdbuffer, char *msgbuffer) {
  //unsigned int stringlen = strlen(cmdbuffer);

  #ifdef DEBUG
  printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
  #endif

  // Send command to server.
  if (SendMessage(socket,cmdbuffer) < 0) {
    Die("Failed to send message");
  }

  #ifdef DEBUG
  printf("[DEBUG] Handling '%s' request to server\n", cmdbuffer);
  #endif

  int bytesRecvd = 0;

  // Read result from server.
  if ((bytesRecvd = ReceiveMessage(socket, msgbuffer)) < 0) {
    Die("Failed to receive message");
  }

  if ((strcmp(msgbuffer, "success") != 0)) {
    // Output the server results.
    printf("%s", msgbuffer);
  }

  // clear msgbuffer
  memset(msgbuffer, 0, sizeof(char)*BUFSIZE);

  #ifdef DEBUG
  printf("--== Received entire result of '%s' from the server --==\n", cmdbuffer);
  #endif

  return 0;
}

int HandleRequestPut(int socket, char *cmdbuffer, char *msgbuffer) {
  //unsigned int stringlen = strlen(cmdbuffer);


  // Getting pointer to the file name, which is the 2nd substring
  char *filename = strchr(cmdbuffer, ' ') + 1;

  #ifdef DEBUG
  printf("[DEBUG] Checking if file exists.\n");
  #endif

  if (FileExists(filename) == 0) {
    #ifdef DEBUG
    printf("[DEBUG] File '%s' exists.\n", filename);
    #endif

    #ifdef DEBUG
    printf("--== Sending message '%s' to the server --==\n", cmdbuffer);
    #endif

    // Send command to server.
    if (SendMessage(socket, cmdbuffer) < 0) {
      Die("Failed to send message");
    }

    #ifdef DEBUG
    printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
    #endif
  
    #ifdef DEBUG
    printf("[DEBUG] Handling '%s' request to server\n", cmdbuffer);
    #endif

    int bytesRecvd = 0;
    // Check for message "Server ready to receive file" message from server.
    if ((bytesRecvd = ReceiveMessage(socket, msgbuffer)) < 0) {
      Die("Failed to receive message");
    }
  
    FILE *file;

    if (strcmp(msgbuffer, "Server ready to receive file") == 0) {
      file = fopen(filename, "r");

      #ifdef DEBUG
      printf("[DEBUG] Received message from server: '%s'\n", msgbuffer);
      #endif

      #ifdef DEBUG
      printf("[DEBUG] Server is ready to receive file\n");
      #endif

      if (file == NULL) {
        printf("Unable to open file '%s'\n", filename);
        return -1;
      }
         
      // Send the file to the server
      while (fgets(msgbuffer, sizeof(char)*BUFSIZE, file) != NULL) {
        #ifdef DEBUG
        printf("[DEBUG] Sending message: '%s'\n", msgbuffer);
        #endif

        // Add a null character for the recv to know when to stop receving a message
        //msgbuffer[strlen(msgbuffer)] = '\0';

        // Probably could use something better then fgets? fgets includes '\n' character at the end
        // replace '\n' at the end of user input with '\0'.
        /*char *pos;
        if ((pos = strchr(msgbuffer, '\n')) != NULL) {
          *pos = '\0';
        }*/

        SendMessage(socket, msgbuffer);

        #ifdef DEBUG
        printf("[DEBUG] Sent message: '%s'\n", msgbuffer);
        #endif     

        // Checking if server has received msg..
        if ((bytesRecvd = ReceiveMessage(socket, msgbuffer)) < 0) {
          Die("Failed to receive message");
        }

        #ifdef DEBUG
        printf("[DEBUG] Sent message: '%s'\n", msgbuffer);
        #endif   

      }

      #ifdef DEBUG
      printf("[DEBUG] end of file, sending null message\n");
      #endif

      // End of file, send a null character message to server to stop receiving.
      //if(feof(file)) {
        memset(msgbuffer, 0, sizeof(char)*BUFSIZE);
        msgbuffer[0] = '\0';
        SendMessage(socket, msgbuffer);
      //}

      fclose(file);

      // Receive a message from server indicating the server has succesfully received the file.
      if ((bytesRecvd = ReceiveMessage(socket, msgbuffer)) < 0) {
        Die("Failed to receive message");
      }

      // Output result from server
      printf("%s\n", msgbuffer);
    } else {
      printf("Received incorrect message from server: '%s'\n", msgbuffer);
      return -1;
    }
  } else {
    printf("File '%s' does not exist in current directory\n", filename);
    return -1;
  }

  // clear msgbuffer
  memset(msgbuffer, 0, sizeof(char)*BUFSIZE);

  #ifdef DEBUG
  printf("--== Received entire result of '%s' from the server --==\n", cmdbuffer);
  #endif

  return 0;
}
