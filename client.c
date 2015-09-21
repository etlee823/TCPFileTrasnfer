#include <stdio.h>			// For printf() and fprintf().
#include <sys/socket.h>		// For socket(), connect(), send(), recv().
#include <arpa/inet.h>		// For sockaddr_in, inet_addr().
#include <stdlib.h>			// For atoi().
#include <string.h>			// For memset(), strstr().
#include <unistd.h>			// For close(), access(), exec().

#define RCVBUFSIZE 1024		// Size of receive message buffer.
#define CMDBUFSIZE 1024		// Size of command buffer.
#define DEBUG 0				// If defined, print statements will be enabled for debugging.


/////////////////////////////////////////////////////////////////////
// Function protoypes.
/////////////////////////////////////////////////////////////////////

// Returns 0 if a string starts with a specfified substring.
int StartsWith(const char *string, const char *substring);

// Prints a message of available commands.
void HelpMessage();

// Returns 0 if the file name specifed exists in the current diretory.
int FileExists(const char *filename);

// Returns 0 if the specified file is succesfully sent to the server.
int PutFileRemote(const char *filename);

// Returns the number of substrings in a string.
int NumInputs(char *string);

// Returns 0 if message sent and server reply are successful.
int HandleRequest(int socket, char *cmdbuffer, char *msgbuffer);


/////////////////////////////////////////////////////////////////////
// Main.
/////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	int sockfd;							// Socket descripter
 	struct sockaddr_in servAddr;		// Server address
	unsigned short servPort;			// Server port
	char *servIP;						// Server IP address (dotted)
	char *string;						// String to send to server
	unsigned int stringLen;				// Length of string
	int bytesRecvd, totalBytesRecvd;	// Bytes to read in single recv() and total bytes read
	int rv;								// Return value of functions
	char cmdbuffer[CMDBUFSIZE];			// Buffer for command
	char msgbuffer[RCVBUFSIZE];			// msgbuffer for string

/*
	// for testing main logic only, skipping socket creation
	#ifdef DEBUG
	goto skip;
	#endif
*/

	// check for correct # of arguments (1 or 2)
	if((argc < 2) ||(argc > 3))
	{
		fprintf(stderr, "Usage: %s <Server IP> [<Port>]\n", argv[0]);
		exit(1);
	}

	// Server IP
	servIP = argv[1];

	// Check if port is specified
	if(argc == 3)
		servPort = atoi(argv[2]);
	else
		servPort = 7; // 7 is a well know port for echo service

	#ifdef DEBUG
	printf("[DEBUG] Creating TCP socket...\n");
	#endif

	// Create a TCP socket
	if((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("socket()");
		exit(1);
	}

	#ifdef DEBUG
	printf("[DEBUG] Created a TCP socket...\n");
	#endif

	// Construct the server address structure
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family		 = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(servIP);	// convert to network byte
	servAddr.sin_port		 = htons(servPort);		// convert to port

	#ifdef DEBUG
	printf("[DEBUG] Connecting to server %s...\n", servIP);
	#endif

	// Connect to the server
	if( (rv = connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr))) < 0)
	{
		perror("connect()");
		exit(1);
	}

	#ifdef DEBUG
	printf("[DEBUG] Connected to server... connect()\n");
	#endif

/*
	// Need to do this differently
	string = "connect\0";
	stringLen = strlen(string);



	#ifdef DEBUG
	printf("--== Sending message '%s' to the server --==\n", string);
	#endif

	// Send a message to the server to connect.
	if((rv = send(sockfd, string, stringLen, 0)) != stringLen)
	{
		//printf("send() sent a different number of bytes than expected.\nError: %i", rv);
		perror("send()");
		exit(1);
	}

	#ifdef DEBUG
	printf("--== Sent message '%s' to the server --==\n", string);
	#endif
*/

/*
	#ifdef DEBUG
	skip:
	#endif
*/

	// Main logic code
	while(1)
	{
		printf("ft> ");
		fgets(cmdbuffer, 100, stdin);

		// replace '\n' at the end of cmdbuffer with '\0'.
		char *pos;
		if((pos=strchr(cmdbuffer, '\n')) != NULL)
			*pos = '\0';

		// NumInputs replaces the buffer used, so create a new buffer.
		char pstring[CMDBUFSIZE];
		strcpy(pstring, cmdbuffer);

		// Check the number of inputs given by the user.
		rv = NumInputs(pstring);

		#ifdef DEBUG
		printf("[DEBUG] rv = NumInputs() = %i\n", rv);
		printf("[DEBUG] cmdbuffer = '%s'\n", cmdbuffer);
		#endif

		// This big if/else could be changed.
		// 1 command input
		if(rv == 1)
		{
			#ifdef DEBUG
			printf("[DEBUG] 1 word input\n");
			#endif

			if(strcmp(cmdbuffer, "quit") == 0)
			{
				stringLen = strlen(cmdbuffer);

				#ifdef DEBUG
				printf("--== Sending message '%s' to the server --==\n", cmdbuffer);
				#endif

				// Send the 'quit' message to the server.
				if((rv = send(sockfd, cmdbuffer, stringLen, 0)) != stringLen)
				{
					//printf("send() sent a different number of bytes than expected.\nError: %i", rv);
					perror("send(quit)");
					exit(1);
				}

				#ifdef DEBUG
				printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
				#endif

				// Exit program.
				break;
			} 
			else if(strcmp(cmdbuffer, "help") == 0)
			{
				HelpMessage();
			} 
			else if(strcmp(cmdbuffer, "ls") == 0)
			{

				#ifdef DEBUG
				printf("[DEBUG] ls command\n");				
				#endif

				stringLen = strlen(cmdbuffer);

				// send ls command to server
				if((rv = send(sockfd, cmdbuffer, stringLen, 0)) != stringLen)
				{
					printf("send() sent a different number of bytes than expected.\nError: %i", rv);
					exit(1);
				}

				#ifdef DEBUG
				printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
				#endif


				#ifdef DEBUG
				printf("[DEBUG] Handling '%s' request to server\n", cmdbuffer);					
				#endif

				// receive ls results from server
				HandleRequest(sockfd, cmdbuffer, msgbuffer);

				#ifdef DEBUG
				printf("--== Received results of '%s' from the server --==\n", cmdbuffer);
				#endif
			}
			else if(strcmp(cmdbuffer, "clear") == 0)
			{
				system("clear");
			}
			else 
			{
				printf("Invalid command.\n");
				HelpMessage();
			}
		} // End of 1 command input.
		// 2 command inputs.
		else if(rv == 2)
		{
			// Basic format validation for option and 1 argument.
			if(StartsWith(cmdbuffer, "get") && strstr(cmdbuffer, " "))
			{
				#ifdef DEBUG
				printf("[DEBUG] get <remote-file> command\n");
				#endif			

				// Send 'mkdir <directory-name>' message.

			} 
			else if(StartsWith(cmdbuffer, "put ") && strstr(cmdbuffer, " "))
			{
				#ifdef DEBUG
				printf("[DEBUG] put <file-name> command\n");
				#endif

				// 1. Verify <file-name> exists

				// Need to extract the file name from cmdbuffer..
				if(FileExists("testfile"))
				{
					#ifdef DEBUG
					printf("[DEBUG] file name: %s does exist\n", cmdbuffer);
					#endif

					// 2. Send file to server through messages
					if((rv = PutFileRemote("<file-name>")) < 0)
						printf("put <file-name> failed with error: %i", rv);
					else 
						printf("%s was successful\n", cmdbuffer);
				}

				// Send 'put <file-name>' message to server.

				// Begin sending messages of the file line by line.

				// Output that the message transfer is complete.

			}
			else if(StartsWith(cmdbuffer, "cd") && strstr(cmdbuffer, " "))
			{
				#ifdef DEBUG
				printf("[DEBUG] cd <directory> command\n");
				#endif

				// Send 'cd <directory-name' message to server.

			}
			else if(StartsWith(cmdbuffer, "mkdir") && strstr(cmdbuffer, " "))
			{
				#ifdef DEBUG
				printf("[DEBUG] mkdir <directory-name> command\n");
				#endif

				// Send 'mkdir <directory-name>' message to server.

			}
			else 
			{
				printf("Invalid command.\n");
				HelpMessage();
			}
		} // End of 2 command input.
		else
		{
			printf("Invalid command.\n");
			HelpMessage();
		}

	} // End while loop.

} // End main.


/////////////////////////////////////////////////////////////////////
// Function implementations.
/////////////////////////////////////////////////////////////////////

int StartsWith(const char *string, const char *substring)
{
	if(strncmp(string, substring, strlen(substring)) == 0)
		return 1;
	return 0;
}

void HelpMessage()
{
	printf("Commands are:\n\n");
	printf("ls:\t\t print a listing of the contents of the current directory\n");
	printf("get <remote-file>:\t\t retrieve the <remote-file> on the server and store it in the current directory\n");
	printf("put <file-name>:\t\t put and store the file from the client machine to the server machine.\n");
	printf("cd <directory-name>:\t\t change the directory on the server\n");
	printf("mkdir <directory-name>:\t\t create a new sub-directory named <directory-name>\n");
}

int HandleRequest(int socket, char *cmdbuffer, char *msgbuffer)
{
	int rv = 0;
	unsigned int stringLen = strlen(cmdbuffer);

	// Send command to server
	if((rv = send(socket, cmdbuffer, stringLen, 0)) != stringLen)
	{
		//printf("send() sent a different number of bytes than expected.\nError: %i", rv);
		perror("(HandleRequest: send(cmdbuffer)");
		exit(1);
	}

	int bytesRecvd = 0;

	// Read results from server until a null terminator is found
	while(1)
	{
		if((bytesRecvd = recv(socket, msgbuffer, RCVBUFSIZE, 0)) < 0)
			perror("HandleRequest: recv(msgbuffer");

		// Check if it is the last message
		if(msgbuffer[0] == '\0')
			break;

		// Output the server results
		printf("%s", msgbuffer);

		// clear msgbuffer
		memset(msgbuffer, 0, sizeof(msgbuffer));
	}

	return 0;
}

int FileExists(const char *filename)
{
	if(access(filename, F_OK) != -1) 
	    return 1;
	// File does not exist
	return 0;
}

int PutFileRemote(const char *filename)
{
	return 0;
}

int NumInputs(char *string)
{
	// send ls command
	#ifdef DEBUG
	printf("[DEBUG] inside NumInputs. String passed was: %s\n", string);
	#endif			

	int numsubstrings = 0;
	char *token = strtok(string, " ");
	while(token)
	{

		#ifdef DEBUG
		printf("[DEBUG] token: %s\n", token);
		#endif		

		numsubstrings++;
		token = strtok(NULL, " ");
	}
	
	return numsubstrings;
}