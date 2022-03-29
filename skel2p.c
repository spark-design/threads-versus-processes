/* Write a multiple concurrent process program that does the following
  1. Prompts the user for the number of integers to enter
  2. Prompts the user for each integer and writes them into
     a file name data.dat
  3. Determines how many integers are > 100
  4. Determines how many integers are < 100
  5. Outputs the total count for each group

  The program should perform this task in the following way:
  Create a producer child that:
  1. Prompts teh user for the number of integers to enter
  2. Prompts the user for each integer and writes them into a file
     named data.dat
  Create a consumer child that:
  1. For each value in the file
      a. Determine if value > 100
      b. If the value > 100, signals the parent with SIGUSR1
  Create a cosnumer child that:
  1. For each value in the file
      a. Determines if value < 100
      b. If the value < 100, signals the parent with SIGUSR2
  Create a parent that:
  1. Creates children 1-3
  2. Pauses
*/

/* include files */
#include <stdio.h>
#include <stdlib.h>

/* Problem 1: Add the necessary include files for this program */
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
  Global variables:
  For this project I am storing the pid's of the three children
  and two integers that serve the role of flags and counters

*/

/* Problem 2: Declare the global variables for pid's of the three
   children and the two integers that serve the role of flags and
   counters. The first flag deals with the large count, the second
   flag deals with the small count.
*/
pid_t cProdPid, cCons1Pid, cCons2Pid, pPid;
struct sigaction saP;
struct sigaction saC;
#define BUF_SIZE 1024
#define SIG_BUF_SIZE 512
int big = 0;
int little = 0;

/* myPrint is a convenience function to use when we are in a signal
   handler. This is because printf uses buffered I/O.
*/
void myPrint(const char *str){
  if( write(STDOUT_FILENO, str, strlen(str)) == -1 ){
    perror("write");
    exit(EXIT_FAILURE);
  }
}

/* Signal handlers
   We will need to implement at least two signal handlers.
   One for the parent and a minimum of one for the children.
*/

/* Problem 3: Define and write the signal handler for the parent */

/* This is the signal handler for the parent.
   This function handles SIGCHLD, SIGUSR1, SIGUSR2.

   On SIGUSR1, increment the counter for large by 1.
   On SIGUSR2, increment the counter for small by 1.
   On SIGCHLD,
     1. Loop while children remain that need to be cleaned up
         a. if the child that is cleaned up is child 1,
	    send child 2 SIGUSR1
	    send child 3 SIGUSR2
     2. if no children remaining, output: larger: large
                                          smaller: small
        and exit the parent.
*/

void P_Signal_Handler(int sig){
  char buf[SIG_BUF_SIZE];
  if( sig == SIGUSR1 ){
    big++;
    return;
  }
  if( sig == SIGUSR2 ){
    little++;
    return;
  }
  if( sig == SIGCHLD ){
    pid_t val;
    while( (val = waitpid( -1, NULL, WNOHANG )) > 0 ) continue;
    if( val == cProdPid || val == 0 ){
      kill(cCons1Pid, SIGUSR1);
      kill(cCons2Pid, SIGUSR2);
    }
  }
}

/* Problem 4: Define and write the signal handler for the children */

/* This is the signal handler for the children.
   This funciton handles SIGUSR1 and SIGUSR2.

   On SIGUSR1, set the first flag to 1.
   On SIGUSR2, set the second flag to 1.
*/

void C_Signal_Handler(int sig){
  if( sig == SIGUSR1 )
    big = 1;
  
  if( sig == SIGUSR2 )
    little = 1;
}

/* Functions for each of the children
   We will be writing functions for each of the three children.
   This should make it easier to answer the questiosn on threads.
*/


/* Problem 5: Define and write functions for child 1. */

/* This is the function for child 1.
   It doesn't require any data nor does it return data.

   This function does the following:

   1. Prompts the user for the number of integers to enter
   2. Opens the file data.dat for write. This should result in
      a new file each time.
   3. Prompt the user to enter each desired integer and write them into
      the file.
   4. Close the file.
*/

void child_1_func(){
  cProdPid = getpid();
  int openFlags = O_CREAT | O_WRONLY | O_TRUNC;
  int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  /* prompting for number of ints to enter */
  int num_ints;
  int in, out, fd;

  /* implement check call later for cleanliness */
  printf("Please enter a number of ints: \n> ");
  scanf("%d", &in);
  printf("%d\n", in);
  fd = creat("data.dat", filePerms);
  if( fd == -1 ){
    perror("Failed to open");
    exit(EXIT_FAILURE);
  }
  unsigned char buf[16]; // to hold ints
  for( int i = 0; i < in; i++ ){
    int input = 0;
    printf("Please enter an integer: ");
    scanf("%d", &input);
    unsigned int mask = 127; // all 8 lower bits turned on
    for( int i = 3; i >= 0; i -- ){
      buf[i] = input & mask;
      mask <<= sizeof(unsigned char) * 8;
    }
    if( write( fd, &buf, sizeof(int) ) == -1 ){
      perror("Failed to write to file");
      exit(EXIT_FAILURE);
    }
  }

  close(fd);
  exit(EXIT_SUCCESS);
}

/* Problem 6: Define and write the function for child 2 */

/* This is the function for child 2.
   It doesn't require any data nor does it return data.

   This function does the following:

   1. Assign a signal handler for SIGUSR1
   2. Loop while the first flag is 0
      a. pause
   3. Open the file data.dat for read
   4. Reads each value from the file
      a. if the value > 100,
         increments the counter
	 sends SIGUSR1 to the parent
   5. Close the file.
   6. Outputs: total larger: counter
*/

void child_2_func(){
  cCons1Pid = getpid();
  if( sigaction( SIGUSR1, &saC, NULL ) == -1 ){
    perror("sa SIGUSR1");
    exit(EXIT_FAILURE);
  }
  while( big == 0 ) pause();
  /* still need to get signal handler */

  int openFlags = O_RDONLY;
  int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int fd;
  unsigned char buf[BUF_SIZE]; // to hold data from the file
  fd = open("data.dat", openFlags | filePerms );
  if( fd == -1 ){
    perror("Failed to write prompt");
    exit(EXIT_FAILURE);
  }
  int value;
  int outBytes;
  
  while( (outBytes = read(fd, buf, BUF_SIZE)) > 0 ){
    // assuming int is 4 bytes, this will read 256 ints at a time, at most
    value = 0;
    // convert the binary data to an integer
    for( int i = 0; i < outBytes; i++ ){
      // we need to reset the value and consume it
      if( i % sizeof(int) == 0 && i > 0 ){
	if( value > 100 ){
	  big ++;
	  kill( getppid(), SIGUSR1 );
	}
	value = 0;
      }
      value <<= (sizeof(unsigned char) * 8); // Move it the necessary bits, in this case, 8
      value += buf[i];
    }
  }

  close(fd);
  printf("Larger: %d\n", big - 1);
  exit(EXIT_SUCCESS);
}

/* Problem 7: Define and write the function for child 3 */

/* This is the function for child 3.
   It doesn't require any data nor does it return data.

   This function does the following:

   1. Assign a signal handler for SIGUSR2
   2. Loop while the second flag is 0
      a. pause
   3. Open the file data.dat for read
   4. Reads each value from the file
      a. If the value < 100,
         increments the counter
	 sends SIGUSR2 to the parent
   5. Close the file
   6. Outputs: total larger: counter
*/

void child_3_func(){
  cCons2Pid = getpid();
  if( sigaction( SIGUSR2, &saC, NULL ) == -1 )
  {
    perror("sa SIGUSR2");
    exit(EXIT_FAILURE);
  }
  while(little == 0) pause();
  /* still need to get signal handler */

  int openFlags = O_RDONLY;
  int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int fd;
  unsigned char buf[BUF_SIZE]; // to hold data from the file
  fd = open("data.dat", openFlags | filePerms );
  if( fd == -1 ){
    perror("Failed to write prompt");
    exit(EXIT_FAILURE);
  }
  
  int value;
  int outBytes;
  while( (outBytes = read(fd, buf, BUF_SIZE)) > 0 ){
    // assuming int is 4 bytes, this will read 256 ints at a time, at most
    value = 0;
    // convert the binary data to an integer
    for( int i = 0; i < outBytes; i++ ){
      // we need to reset the value and consume it
      if( i % sizeof(int) == 0 && i > 0 ){
	if( value < 100 ){
	  little++;
	  kill( getppid(), SIGUSR1 );
	}
	value = 0;
      }
      value <<= (sizeof(unsigned char) * 8); // Move it the necessary bits, in this case, 8
      value += buf[i];
    }
  }

  close(fd);
  printf("Smaller: %d\n", little - 1);
  exit(EXIT_SUCCESS);
}

/* This function forks a child and runs the function passed
   in affter the child has successfully forked. I have provided
   it to make the code easier to read.
*/

pid_t hndlFork(void (*child)(void)){
  pid_t p;
  p = fork();
  if( p == -1 ){
    perror("fork");
    exit(EXIT_FAILURE);
  }
  if( p == 0 ){
    child();
  }
  return p;
}

/* Problem 8: Define and write the function main */

/* This is the function for function main prior to calling fork
   and the parent after calling fork.
   The function does the following:
   1. Assign a signal handler for SIGCHLD
   2. Fork child 2 and child 3
      You can do this by calling hndlFork as follows:
      child = hndlFork(childFcn);
      where child stores a pid and childFcn is the
      function you have written to handle that child
   3. Assign a signal handler for SIGUSR1, SIGUSR2
   4. Fork child 1
   5. Loop forever
      a. pause
*/

int main(int argc, char *argv[]){
  pPid = getpid();
  saC.sa_handler = C_Signal_Handler;
  saC.sa_flags = 0;

  saP.sa_handler = P_Signal_Handler;
  saP.sa_flags = 0;

  if( sigaction( SIGCHLD, &saP, NULL ) == -1 )
  {
    perror("saP SIGCHLD");
    exit(EXIT_FAILURE);
  }
  cCons1Pid = hndlFork( &child_2_func );
  cCons2Pid = hndlFork( &child_3_func );

  if( sigaction( SIGUSR1, &saP, NULL ) == -1 )
  {
    perror("saP SIGUSR1");
    exit(EXIT_FAILURE);
  }
  if( sigaction( SIGUSR2, &saP, NULL ) == -1 )
  {
    perror("saP SIGUSR2");
    exit(EXIT_FAILURE);
  }

  cProdPid = hndlFork( &child_1_func );
  while(1) pause();
  
  exit(EXIT_SUCCESS);
}
