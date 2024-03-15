/*
Name: Tyler Gebel
Assignment: OTP - enc_server
Date: 3-15-24
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>


/*********************************************************
GLOBAL VARIABLES
**********************************************************/
char buffer[8000];
int fileSize, charsRead, charsSent;
char msgBuff[100000];
char keyBuff[100000];

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
int rcvSize(int socket);
void rcvMsgInput(int socket, int size);
void rcvKeyInput(int socket, int size);
void encMsg(char message[], char key[], int size);
int convertInt(int curInt);




/*********************************************************
FUNCTION: int main
ARGUMENTS: int argc
           char *argv[]
**********************************************************/


int main(int argc, char *argv[]){
  int connectionSocket;

  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo;
  pid_t pid = -5;

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  else {
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

  // Start listening for connections. Allow up to 5 connections to queue up
  listen(listenSocket, 5);

  while (1) {
    // Start listening for connections. Allow up to 5 connections to queue up
    //listen(listenSocket, 5);
     
    // Accept a connection, blocking if one is not available until one connects
    // Get the size of the address for the current client that will connect
    sizeOfClientInfo = sizeof(clientAddress);

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

   pid = fork();
   if (pid == -1) {
   error("Error in establishing fork.\n");
   exit(1);
   }
   if (pid == 0) {
      if (clientConnectionConfirm(connectionSocket)) {
          char return_conf[] = "enc_val";
          int conf_length = strlen(return_conf);
        
          //Send the confirmation that the sever got the validation string
          charsSent = send(connectionSocket, return_conf, conf_length, 0);


          fileSize = rcvSize(connectionSocket);
          if (fileSize == 0) {
            fprintf(stderr, "File size is zero.");
            }
          else if (fileSize < 0) {
          fprintf(stderr,"Error retrieving the file size");
            }
          char size_conf[] = "confirmSize";
          int size_conf_length = strlen(size_conf);
          charsSent = send(connectionSocket, size_conf, size_conf_length, 0);

          rcvMsgInput(connectionSocket, fileSize);
          if (sizeof(msgBuff) < fileSize) {
            fprintf(stderr, "Could not get messge input");
          }
          charsSent = 0;
   
          rcvKeyInput(connectionSocket, fileSize);
          if (sizeof(keyBuff) < fileSize) {
            fprintf(stderr, "Could not get key input.");
          } 
        
          encMsg(msgBuff, keyBuff, fileSize);

          // Get the message from the client and display it
          memset(buffer, '\0', sizeof(buffer));
        
          //We reset charsSent and charsRead to 0
          charsSent = 0;
          charsRead = 0;

          // The encrypted message buffer is sent to the client until charsSent is equal
          // to the fileSize variable. The length of the message buffer is kept track of
          // through each iteration. Once all bytes are sent, the child process exits

          while (charsSent < fileSize) {
            int msgLength = strlen(msgBuff);
            charsSent += send(connectionSocket, msgBuff, msgLength, 0);
            }
        exit(0);
      }

      else {
       char invalid[] = "Invalid connection";
       charsRead = send(connectionSocket, invalid, sizeof(invalid), 0);
       fprintf(stderr, "The connection to the clinet is invalid\n");
       exit(1);
       break;
       }
     exit(0);
     //close(connectionSocket);
    
    // The parent process will continue on with other client sockets instead of
    // waiting on the current client socket child process
    int pidExitStatus;
    waitpid(pid, &pidExitStatus, WNOHANG);
    close(connectionSocket);
   }
} 
  // Close the listening socket
  close(listenSocket); 
  return 0;
  }
  }



/***********************************************************************
FUNCTION: int clientConnectionConfirm

ARGUMENTS:
int socket - potential incoming socket file descriptor
that we want to confirm is the correct socekt from current
client in child process

RETURNS: Returns 1 if the client is properly connected to the server
or 0 if there was an error or the socket file descriptor was not the
correct one
************************************************************************/

int clientConnectionConfirm(int socket) {
  char incoming_buff[100];
  int char_rcv = 0;

  //Zero out entire buffer
  memset(incoming_buff, '\0', sizeof(incoming_buff));
  while (char_rcv == 0) {
    char_rcv = recv(socket, incoming_buff, sizeof(incoming_buff) - 1, 0);
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



/****************************************************************************
FUNCTION: int rcvSize

ARGUMENTS:
int socket - client socket file descriptor that can send a file over of a
variable size

RETURNS: Returns the number of bytes that the socekt has sent over through the
buffer. We can assign the result of the rcv() function to a variable and convert
that variable to an integer to get the size of the file sent through the socket
*******************************************************************************/
int rcvSize(int socket) {
  //Clear the buffer
  memset(buffer, '\0', sizeof(buffer));
  while (charsRead == 0) {
    //We get the size of the buffer - 1 to not account for newline character
    charsRead = recv(socket, buffer, sizeof(buffer) - 1, 0);
    }
    fileSize = atoi(buffer);
    memset(buffer, '\0', sizeof(buffer));
    return fileSize;

}

/****************************************************************************
FUNCTION: void rcvMsgInput

ARGUMENTS:
int socket - client socket file descriptor that can send a file over of the
expected size

int size - the size expected of the message being sent over

RETURNS: Nothing, but the message buffer is filled with the incoming message
*****************************************************************************/
void rcvMsgInput(int socket, int size) {
  int bytes = 0;
  int byteTotal = 0;

  memset(msgBuff, '\0', sizeof(msgBuff));
  while (byteTotal < size) {
    // The buffer is cleared each time in order to get another section of bytes from the client socket
    memset(buffer, '\0', sizeof(buffer));
    bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
    byteTotal += bytes;
    // We add the section of bytes to the msgBuff
    strcat(msgBuff, buffer);
  }
  return;
}


/****************************************************************************
FUNCTION: void rcvKeyInput

ARGUMENTS:
int socket - client socket file descriptor that can send a file over of the
expected size

int size - the size expected of tbe  key being sent over

RETURNS: Nothing, but the key buffer is filled with the incoming key message
*****************************************************************************/
void rcvKeyInput(int socket, int size) {
  int bytes = 0;
  int byteTotal = 0;

  memset(keyBuff, '\0', sizeof(keyBuff));
  while (byteTotal < size) {
    memset(buffer, '\0', sizeof(buffer));
    bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
    byteTotal += bytes;
    strcat(keyBuff, buffer);
  }
  return;
}


/*****************************************************************************
FUNCTION: void encMsg

ARGUMENTS:
char message[] - a buffer that conatains the data for the message the client
socket has sent

char key[] - a buffer that contains the data for the key the client socket has
sent

int size - the size in bytes of the message and key sent by the client socket.
As expected, the message and key need to be the same size

RETURNS: Nothing, but the message buffer now contains the encrypted message
******************************************************************************/
void encMsg(char message[], char key[], int size) {

  // We start at the first capital letter A, which is 0 for our numerical value
  int startLetter = 'A', msgChar, keyChar;

  for (int i = 0; i < size; i++) {

    //We convert the characters in both the message and the key to integers
    msgChar = (int)message[i];
    keyChar = (int)key[i];
    
    //We are looking at ASCII Table values now, so if the integer is 10, it is a new line character,
    //so we are at the end of the message and do not want to look at the new line character
    if (msgChar != 10) {
      msgChar = convertInt(msgChar) + convertInt(keyChar);
      msgChar = msgChar % 27;

      //We have converted the integer to the correct integer and then used modulo 27 to convert to the right
      //numerical value. If the integer is 26, the numerical value is a space character. Otherwise, we take the
      //converted integer and add the start letter, which ASCII value is 65, to it to get the correct capitol letter
      //as a ASCII value. This is then converted back to a character and assigned to the correct inedex of the message
      //buffer

      if (msgChar == 26) {
        message[i] = ' ';
      }
      else {
        msgChar = startLetter + msgChar;
        message[i] = (char)msgChar;

          }

      }

      }
  return;

}

/*************************************************************************************
FUNCTION: int convertInt

ARGUMENTS:
int curInt - The current character in that has been converted to an integer and needs
to be the correct value for encrypting the message

RETURNS: The converted character to intgeger value is returned as the correct integer
value for caluclation purposes
**************************************************************************************/
int convertInt(int curInt) {
  if (curInt == 32) {
    curInt = 26;
  }
  else {
    curInt = curInt - 65;
  }
  return curInt;
}

