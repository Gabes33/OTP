/*
Name: Tyler Gebel
Assignment: OTP - dec_client
Date: 3-17-24
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
char buffer[3000] = {0};
char decMsg[100000] = {0};


/****************************************************************
FUNCTION DECLARATIONS
*****************************************************************/
int confirmServer(int socket);
int checkLength(char strFile[]);
void sendFile(int socket, char strFile[], int length);
void validateFile(char strFile[], int length);
void rcvDecryptMsg(int socket, int length);


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
  struct hostent* hostInfo = gethostbyname("localhost"); 
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
  if (argc < 4) { 
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    exit(1); 
  } 

  portNumber = atoi(argv[3]);
  
  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, portNumber, "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  //Confirm connection with server is correct
  int checkConnection = confirmServer(socketFD);
  if (checkConnection == 2) {
    fprintf(stderr, "Client connection to Server refused or invalid");
    exit(2);
  }
  else if (checkConnection == 1) {
    
    int msgLength = checkLength(argv[1]);
    int keyLength = checkLength(argv[2]);
    if (msgLength > keyLength) {
        error("Message and key size are not equal");
        }
    validateFile(argv[1], msgLength);
    validateFile(argv[2], keyLength);

    // Get input message from user
    // printf("CLIENT: Enter text to send to the server, and then hit enter: ");

   // Clear out the buffer array
    memset(buffer, '\0', sizeof(buffer));

    //We want to send the size of the message and key to the server socket. This should
    //be sent as a string
    sprintf(buffer, "%d", msgLength);
    charsWritten = send(socketFD, buffer, sizeof(buffer), 0);
    memset(buffer, '\0', sizeof(buffer));
  
    charsRead = 0;
    //We wait to recieve the correct confirmation message that the server recieved the size
    while (charsRead == 0) {
      charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    }
    if (strcmp(buffer, "confirmSize") != 0) {
      fprintf(stderr, "There was an error confirming the size with the server\n");
    }
    memset(buffer, '\0', sizeof(buffer));

    //We send the message file contents to the server
    sendFile(socketFD, argv[1], msgLength);
    
    memset(buffer, '\0', sizeof(buffer));
    
    // We send the key file contents to the server
    sendFile(socketFD, argv[2], keyLength);
    
    memset(buffer, '\0', sizeof(buffer));

    //Receive the decrypted message and redirect it to stdout
    rcvDecryptMsg(socketFD, msgLength);
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
    char validation[] = "dec_val";
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
    if (strcmp(buffer, "dec_val") != 0) {
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
  int fe = open(strFile, O_RDONLY);
  if (fe < 0) {
    error("Could not successfully open file");
  }
  // We look for the offset of the file and return the number of bytes to this offset
  int fileLength = lseek(fe, 0, SEEK_END);
  close(fe);
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
 memset(buffer, '\0', sizeof(buffer));

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


/*************************************************************************************
FUNCTION: void validateFile

ARGUMENTS:

char strFile[] - the file that is recieved to be validated. The function checks
for valid capital letters A through Z and and the space character. If any other
character is found, and error is returned. If the end of the file is reached, the
function returns

int length - the length the file is expected to be, whether it is the message file
or the key file

RETURNS: Nothing, but if the file is valid, the function will just return instead of
returning an error
**************************************************************************************/
void validateFile(char strFile[], int length) {
  
  FILE *file = fopen(strFile, "r");
  if (file == NULL) {
    error("Error occured when opening the file");
  }
  

  for (int i = 0; i < length; i++) {
    
    int curChar = fgetc(file);

    if ((curChar >= 65) && (curChar <= 90)) {
      continue;
    }
    else if (curChar == 32) {
      continue;
    }
    else if (curChar == 10) {
      break;
    }
    else {
      error("Invalid character or characters in given file");
    }
  }
  fclose(file);
  return;


}


/********************************************************************************************
FUNCTION: void rcvDecryptMsg

ARGUMENTS:

int socket - the server socket file descriptor that the client socket is connected to

int length - the expected length of the decyrpted message, which will be the same length as
the message and key files

RETURNS: Nothing, but the global encMsg buffer will contain the decrypted message
*********************************************************************************************/
void rcvDecryptMsg(int socket, int length) {

  memset(decMsg, '\0', sizeof(decMsg));
  memset(buffer, '\0', sizeof(buffer));
  
  int totalBytes = 0;
  int bytes = 0;

  // We iterate until the total bytes are one less than the length to account for the newline character
  while (totalBytes < length - 1) {
    bytes = 0;
    bytes = recv(socket, buffer, sizeof(buffer), 0);
    
    //We want to add the bytes in buffer to the message buffer as a string
    sprintf(buffer, "%s", buffer);
    totalBytes += bytes;

    //We now add the converted string to the messsage buffer
    strcat(decMsg, buffer);
    memset(buffer, '\0', sizeof(buffer));
  }
  strcat(decMsg, "\0");
  printf("%s", decMsg);
  return;

}

