/* Write a threaded program that does the following
   1. Prompts the user for the number of integers to enter
   2. Prompts the user for each integer and writes them into
      a file named data.dat
   3. Determines how many integers are > 100
   4. Determines how many integers are < 100
   5. Outputs the total count for each group.

   The program should perform this task in the following way:
   Create a producer thread that:
   1. Prompts the user for the number of integers to enter
   2. Prompts the user for each integer and writes them into a file
      named data.dat
   Create a consumer thread that:
   1. Determines how many integers are > 100
   2. Outputs the value
   3. Sets that value to its exit value
   Create a consumer thread that:
   1. Determines how many integers are < 100
   2. Outputs that value
   3. Sets that value to its exit value
   Create a main thread that:
   1. Creates threads 1-3
   2. Waits on the values of threads 2 and 3
   3. Outputs the values from threads 2 and 3
*/

/* include files */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
/* Problem 1: Add the necessary include files for this program */


/* 
   Global variables:
   We will need a mutex, a condition variable, and a predicate variable.
   Recall that the predicate variable is the variable we use to determine
   whether data was available prior to our first call to pthread_cond_wait
*/

/* Problem 2: Declare the global variables for the predicate variable,
   the mutex, and the condition variable. Do not forget to initialize
   the mutex and the condition variable.
*/
#define BUF_SIZE 1024
int pred;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t threads[3];

/* This is a convenience function for dealing with errors
   and threads
*/

void hndlError(int error, const char *str){
  if( error == 0 ) return;
  errno = error;
  perror(str);
  exit(EXIT_FAILURE);
}

/* Define the three thread start functions.
   you can name them whatever you wish
*/

/* Problem 3: Define and write the start function for thread 1 */

/* This is the start function for thread 1
   This function does the following:

   1. Detaches itself
   2. Prompts the user for the number of integers to enter
   3. Locks the mutex
   4. Opens the file data.dat for write. This should result in
      a new file each time.
   5. Prompt the user to enter each desired integer and write them into
      the file.
   6. Close the file
   7. Set the predicate variable to 1
   8. Unlock the mutex
   9. Signal the condition variable
   10. Exit the thread with a value of NULL
*/

void *Thread_1_Function(){
  int value;
  char buf[16]; // to hold any int
  int openFlags = O_CREAT | O_WRONLY | O_TRUNC;
  int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int fd;
  int number;
  if( pthread_detach(threads[0]) != 0) {
    perror("Failed to detach\n");
  }
  printf("Please enter a number of ints\n");
  scanf("%d", &number);
  pthread_mutex_lock(&lock);
  fd = creat("data.dat",filePerms);
  if(fd == -1) {
    perror("failed to open"); exit(EXIT_FAILURE);
  }

  for(int i = 0;i < number; i++)
  {
    printf("Enter an integer: \n");
    scanf("%d" , &value);
    printf("Thread 1: %d,", value);
    int mask = 127; // bitmask with all first 8 bits turned on
    for( int i = 3; i >= 0; i-- ){
      buf[i] = value & mask;
      mask <<= sizeof(char) * 8; //
    }
    if( write(fd,&buf,sizeof(int)) == -1){
      perror("failed to write to file");
      exit(EXIT_FAILURE);
    }
  }
  close(fd); 
 
  pred = 1;
  pthread_mutex_unlock(&lock);
  pthread_cond_signal(&cond1);

  pthread_exit(NULL);
}

/* Problem 4: Define and write the start function for thread 2 */

/* This is the start function for thread 2.
   This function does the following:

   1. Declare and allocate space for the counter
   2. Locks the mutex
   3. Loops while the predicate variable is 0
      a. waits on the condition variable and the mutex
   4. Opens the file data.dat for read
   5. Unlocks the mutex
   6. Reads each value from the file
      a. If the value is > 100, increments the counter.
   7. Closes the file
   8. Outputs: total larger: counter
   9. Exit the thread with a value of the counter
*/

void * Thread_2_Function()
{
  int value;
  char tmp_buf[BUF_SIZE];
  int openFlags = O_RDONLY;
  int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int count = 0;
  pthread_mutex_lock(&lock);
  while(pred == 0) {
    pthread_cond_wait(&cond1,&lock);
  }
  int fd;
  fd = open("data.dat",openFlags|filePerms);
  if (fd == -1) {
    perror("failed to write prompt");
    exit(EXIT_FAILURE);
  }
  pthread_mutex_unlock(&lock);
  int OutBytes;
  while( (OutBytes = read(fd, tmp_buf, BUF_SIZE)) > 0 )
  {
    // assuming int is 4 bytes, this will read 256 ints at a time, at most
    value = 0;
    // convert the binary data to an integer
    for( int i = 0; i < OutBytes; i++ ){
      // we need to reset the value and consume it
      if( i % 4 == 0 && i > 0 ){
	if( value > 100 )
	  count++;
	printf("Thread2: value: %d\n", value);
      }
      value <<= (sizeof(char) * 8); // Move it the necessary bits, in this case, 8
      value += tmp_buf[i];
    }
  }
  close(fd);
  printf("Total Count Larger: %d\n",count);

  pthread_exit(&count);

}

/* Problem 5: Define and write the start function for thread 3 */

/* This is the start function for thread 3.
   This function does the following:

   1. Declare and allocate space for the counter
   2. Locks the mutex
   3. Loops while the predicate variable is 0
      a. waits on the condition variable and the mutex
   4. Opens the file data.dat for read
   5. Unlocks the mutex
   6. Reads each value from the file
      a. If the value is < 100, increments the counter
   7. Closes the file
   8. Outputs: total smaller: counter
   9. Exit the thread with a value of the counter
*/

void * Thread_3_function()
{
  int value;
  char tmp_buf[BUF_SIZE];
  int openFlags = O_RDONLY;
  int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int count = 0;
  pthread_mutex_lock(&lock);
  while(pred == 0) {
    pthread_cond_wait(&cond1,&lock);
  }
  int fd;
  fd = open("data.dat",openFlags|filePerms);
  if (fd == -1) {
    perror("failed to write prompt");
    exit(EXIT_FAILURE);
  }
  pthread_mutex_unlock(&lock);
  int OutBytes;

  while( (OutBytes = read(fd, tmp_buf, BUF_SIZE)) > 0 )
  {
    // assuming int is 4 bytes, this will read 256 ints at a time, at most
    value = 0;
    // convert the binary data to an integer
    for( int i = 0; i < OutBytes; i++ ){
      // we need to reset the value and consume it
      if( i % 4 == 0 && i > 0 ){
	if( value < 100 )
	  count++;
	printf("Thread3: value: %d\n", value);
      }
      value <<= (sizeof(char) * 8); // Move it the necessary bits, in this case, 8
      value += tmp_buf[i];
    }
  }
  close(fd);
  printf("Total Count Smaller: %d\n",count);
  pthread_exit(&count);
  
}

/* Problem 6: Define and write the function for the main thread */

/* This is the function for the main thread (i.e. main)
   The function does the following:
   1. Create threads 1-3.
      We don't know what order they will execute in, but create them
      in the order 2, 3, 1 for this problem.
   2. Join thread 2 and store its return value.
      This should be the total number of values > 100
   3. Join thread 3 and store its return value.
      This should be the total number of values < 100
   4. Output: larger: return value from thread 2
              smaller: return value from thread 3
*/

int main(int argc, char *argv[]){
  int larger, smaller, my_err;
  my_err = pthread_create(&(threads[1]), NULL , &Thread_2_Function,NULL);
  hndlError(my_err,"Create 2");
  my_err = pthread_create(&(threads[2]),NULL,&Thread_3_function,NULL);
  hndlError(my_err,"create 3");
  my_err = pthread_create(&(threads[0]),NULL,&Thread_1_Function,NULL);
  hndlError(my_err,"create 1");

  larger = pthread_join(threads[1],NULL);
  smaller = pthread_join(threads[2],NULL);
  printf("Larger: %d\n",larger);
  printf("Smaller: %d",smaller);
  exit(EXIT_SUCCESS);
}
