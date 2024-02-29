/*
Name: Tyler Gebel
Assignment: OTP - Keygen
Date: 2-29-24
*/




#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


int main(int argc, char* argv[]) {
  int character = 'A'; // This is used as starting ASCII Value
  int count = 0;

  if (argc == 2) {
    int length = atoi(argv[1]);
    while (count < length) {
     int key = rand() % 27;
     if (key == 26) {
       printf(" ");  // This is the space character
     }
     else if (key < 26 && key > 0) {
       key = key + character;
       printf("%c", key);   // The character is immediately printed out after it is calculated
       }
     count++;
    }
    printf("\n");
    return 0;
  }
  else if (argc > 2) {
    fprintf(stderr, "Too many arguments.");
  }
  else {
    exit(1);
  }

}
