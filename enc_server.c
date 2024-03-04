/*
Name: Tyler Gebel
Assignment: OTP - enc_server
Date: 2-28-24
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;

  // Store the port number
  address->sin_port = htons(portNumber);

  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

/*********************************************************
FUNCTION DECLARATIONS
**********************************************************/
int clientConnectionConfirm(int socket);








/*********************************************************
FUNCTION: int main
ARGUMENTS: int argc
           char *argv[]
**********************************************************/


int main(int argc, char *argv[]){
  int connectionSocket, charsRead, charsSent, fileSize;
  char buffer[256];

  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  pid_t pid = -5;

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){

    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept");
    }

    printf("SERVER: Connected to client running at host %d port %d\n", 
                          ntohs(clientAddress.sin_addr.s_addr),
                          ntohs(clientAddress.sin_port));

    int pid = fork();
    if (pid == -1) {
      error("Error in establishing fork.\n");
      exit(1);
    }
    if (pid == 0) {
      if (clientConnectionConfirm(connectionSocket)) {
        char return_conf[] = "enc_val";
        int conf_length = strlen(return_conf);
        charsRead = send(connectionSocket, return_conf, conf_length, 0);

        
      }
    // Get the message from the client and display it
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    charsRead = recv(connectionSocket, buffer, 255, 0); 
    if (charsRead < 0){
      error("ERROR reading from socket");
    }
    printf("SERVER: I received this from the client: \"%s\"\n", buffer);

    // Send a Success message back to the client
    charsRead = send(connectionSocket, 
                    "I am the server, and I got your message", 39, 0); 
    if (charsRead < 0){
      error("ERROR writing to socket");
    }
    // Close the connection socket for this client
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}




}


/***********************************************************************
FUNCTION: clientConnectionConfirm

ARGUMENTS: int socket - potential incoming socket file descriptor
that we want to confirm is the correct socekt from current
client in child process

RETURNS: Returns 1 if the client is properly connected to the server
or 0 if there was an error or the socket file descriptor was not the
correct one
************************************************************************/

int clientConnectionConfirm(int socket) {
  char incoming_buff[99];
  int char_rcv = 0;

  //Zero out entire buffer
  memset(incoming_buff, '\0', sizeof(incoming_buff));
  while (char_rcv == 0) {
    char_rcv = recv(socket, incoming_buff, sizeof(incoming_buff), 0);
    if ((strcmp(incoming_buff, "enc_val")) == 0) {

      //string received back is equal to confirmation phrase, so return true
      return 1;
  }
    else {
      return 0;
    }
    memset(incoming_buff, '\0', sizeof(incoming_buff));
  }
 return 0;
}

