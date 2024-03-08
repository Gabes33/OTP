/*
Name: Tyler Gebel
Assignment: OTP - enc_client
Date: 3-4-24
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()



/***************************************************************
GLOBAL VARIABLES
****************************************************************/
int charsWritten, charsRead;
char buffer[256];
char msgBuff[1000];
char keyBuff[1000];
char encMsg[1000];


/****************************************************************
FUNCTION DECLARATIONS
*****************************************************************/
int confirmServer(int socket);
int checkLength(char strFile[]);
void storeFile(char rcvBuff[], char strFile[]);


/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(1); 
} 



// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);



  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

/*******************************************************************
FUNCTION: int main

ARGUMENTS: 
int argc
char *argv[]

********************************************************************/

int main(int argc, char *argv[]) {
  int socketFD, portNumber;
  struct sockaddr_in serverAddress;


  // Check usage & args
  if (argc < 3) { 
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    exit(1); 
  } 

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[2]), argv[1]);

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  //Confirm connection with server is correct
  int checkConnection = confirmServer(socketFD);
  if (checkConnection == 2) {
    exit(2);
  }
  else if (checkConnection == 1) {
    
    int msgLength = checkLength(argv[1]);
    int keyLength = checkLength(argv[2]);
    if (msgLength != keyLength) {
        error("Message and key size are not equal");
        }

    // Get input message from user
    // printf("CLIENT: Enter text to send to the server, and then hit enter: ");

   // Clear out the buffer array
    memset(buffer, '\0', sizeof(buffer));

    //We want to send the size of the message and key to the server socket. This should
    //be sent as a string
    sprintf(buffer, "%d", msgLength);
    charsWritten = send(socketFD, buffer, sizeof(buffer), 0);
    memset(buffer, '\0', sizeof(buffer));

    while (charsRead == 0) {
      charsRead = rcv(socketFD, buffer, sizeof(buffer) - 1, 0);
    }
    
    if strcmp(buffer, "confirm size") != 0)


    // Get input from the user, trunc to buffer - 1 chars, leaving \0
    //fgets(buffer, sizeof(buffer) - 1, stdin);
    // Remove the trailing \n that fgets adds
    buffer[strcspn(buffer, "\n")] = '\0'; 

    // Send message to server
    // Write to the server
    charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
    if (charsWritten < 0){
      error("CLIENT: ERROR writing to socket");
    }
    if (charsWritten < strlen(buffer)){
      printf("CLIENT: WARNING: Not all data written to socket!\n");
    }

    // Get return message from server
    // Clear out the buffer again for reuse
    memset(buffer, '\0', sizeof(buffer));
    // Read data from the socket, leaving \0 at end
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
    if (charsRead < 0){
      error("CLIENT: ERROR reading from socket");
    }
    printf("CLIENT: I received this from the server: \"%s\"\n", buffer);

  }
  // Close the socket
  close(socketFD); 
  return 0;
}


/****************************************************************
FUNCTION: int confirmServer

ARGUMENTS: 
int socket - potential incoming socket file descriptor
that we want to confirm is the correct server socket

RETURNS: Returns 1 if the server is properly connected to the client
or 0 if there was an error or the socket file descriptor was not the
correct one


*****************************************************************/


int confirmServer(int socket) {
    char validation[] = "enc_val";
    int val_length = strlen(validation);
    charsWritten = send(socket, validation, val_length, 0);
    memset(buffer, '\0', sizeof(buffer));
    if (charsWritten < 0) {
      error("Client unable to send message on socket");
      return 0;
    }
    charsRead = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0) {
      error("Client unable to read message on socket");
      return 0;
    }
    if (strcmp(buffer, "enc_val") != 0) {
      fprintf(stderr, "Invalid client on socket connection");
      return 2;
    }

   return 1;

}

/********************************************************************
FUNCTION: int checkLength

ARGUMENTS:
char strFile[] - String file that can either be the message or the key that has
been generated. We open this file and check the length.

RETURNS: The length size is returned as an int.
**********************************************************************/
int checkLength(char strFile[]) {
  int file = open(strFile, O_RDONLY);
  if (file < 0) {
    error("Could not successfully open file");
  }
  int fileLength = lseek(file, 0, SEEK_END); // We look for the offset of the file and return number of bytes to this offset
  close(file);
  return fileLength;

}


/*********************************************************************
FUNCTION: void sendFile

ARGUMENTS:
int socket - the file descriptor associated with the server socket the client
socket is connected to

char strFile[] - String file that can either be message or key file that has been
generated. We will open this file for reading and then sending all in one sweep

int length - the length that we expect the file, which is the message or the key,
to be

RETURNS: Nothing, but the file has been read and sent to the server socket through
the buffer
***********************************************************************/
void sendFile(int socket, char strFile[], int length){

 memset(buffer, '\0', sizeof(buffer));
 int curBytes = 0;
 int strF = open(strFile, O_RDONLY);
 
 int totalBytes = 0;
 while (totalBytes < length) {
   memset(buffer, '\0', sizeof(buffer));

   //We read the file without accounting for the newline character
   curBytes = read(strF, buffer, sizeof(buffer) -1);
   int buffLength = strlen(buffer);

   //Now we will send the current set of bytes over.
   totalBytes += send(socket, buffer, buffLength, 0);
 }
 memset(buffer, '\0', sizeof(buffer));
 close(strF);
 return;


}

