#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#define BUFFER_SIZE 5
#define MAX_THREADS 32768

void *producer(void *param);
void *consumer(void *param);

/* Shared variables */
int maxSleepTime = 0;
int buffer[BUFFER_SIZE] = {-1, -1, -1, -1, -1};
int in = 0;
int out = 0;

/* Thread stats */
int totalProduced = 0;
int totalConsumed = 0;
int producePerThread[MAX_THREADS];
int consumePerThread[MAX_THREADS];

/* Shared flags */
int exitFlag = 0;
int showBufferStats = 0;

/* Misc stats */
int timesFull = 0;
int timesEmpty = 0;

pthread_mutex_t mutex;
sem_t empty;
sem_t full;

//********************************************************************
//
// Get Buffers Occupied
//
// This function returns the amount of buffers occupied when the index
// is not equal to -1.
//
// Return Value
// ------------
// int        Amount of occupied buffers by a number not -1
//*******************************************************************

int get_buffers_occupied() {
  int buffersOccupied = 0;
  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    if (buffer[i] != -1) 
    {
      buffersOccupied++;
    }
  }

  return buffersOccupied;
}

//********************************************************************
//
// Is Buffer Full
//
// This function determines if the buffer is full by comparing the amount of occupied
// buffers to the specified BUFFER_SIZE.
// 
//
// Return Value
// ------------
// int        Returns 1 if the buffer is full and 0 if the buffer has spaces avaliable
//*******************************************************************

int is_buffer_full() {
  if (get_buffers_occupied() == BUFFER_SIZE) {
    return 1;
  }

  return 0;
}

//********************************************************************
//
// Is Buffer Empty
//
// This function determines if the buffer is empty by comparing the amount of occupied
// buffers to zero.
// 
//
// Return Value
// ------------
// int        Returns 1 if the buffer has zero occupied buffers and 0 if the buffer has more than zero occupied buffers
//*******************************************************************

int is_buffer_empty() {
  if (get_buffers_occupied() == 0) {
    return 1;
  }

  return 0;
}


//********************************************************************
//
// Buffer Insert Item
//
// This function will take a value and insert it into the buffer and increment
// the amount produced for the specified thread id and the global total produced amount.
// 
//
// Value Parameters
// ------------
// tid		        int		The thread index
// nextProduced		int		The generated value to store into the buffer from the producer runner
//*******************************************************************

void buffer_insert_item(int tid, int nextProduced) {
  buffer[in] = nextProduced;
  in = (in + 1) % BUFFER_SIZE;
  
  producePerThread[tid]++;
  totalProduced++;
}

//********************************************************************
//
// Buffer Remove Item
//
// This function will remove a value from the buffer and increment the amount consumed
// for the specified thread id and the global total consumed amount.
// 
//
// Value Parameters
// ----------------
// tid		        int		The thread index
// nextConsumed		int		The value to consume and remove from the buffer provided by the consumer runner
//*******************************************************************

void buffer_remove_item(int tid, int nextConsumed) {
  buffer[out] = -1;

  out = (out + 1) % BUFFER_SIZE;
  consumePerThread[tid]++;
  totalConsumed++;
}

//********************************************************************
//
// Print Buffer
//
// This function will print the amount of occupied buffers and the
// current buffer and all of it's indexes and where the next write
// and read is going to occur.
//*******************************************************************

void print_buffer() {
    printf("(buffers occupied: %d)", get_buffers_occupied());

    printf("\nbuffers: ");
    int i;

    for(i = 0; i < BUFFER_SIZE; i++)
    {
        printf("%d   ", buffer[i]); 
    }

    printf("\n");
    printf("        ---- ---- ---- ---- ----\n");
    printf("         ");

    for(i = 0; i < BUFFER_SIZE; i++)
    {
      if (in == i && out == i) {
        printf("WR    ");
      } else if (in == i) {
        printf("W    ");
      } else if (out == i) {
        printf("R    ");
      } else {
        printf("     ");
      }
    }

    printf("\n\n");
}

//********************************************************************
//
// Print Simulation Stats
//
// This function prints the entire calculated simulation stats which
// includes the simulation time, max sleep time, number of producer and consumer threads,
// the size of buffer, per thread statistics for produce/consume amount, and number of times the buffer was empty/full.
//*******************************************************************

void print_simulation_stats(int simTime, int producerThreads, int consumerThreads) {
  printf("\n\nPRODUCER / CONSUMER SIMULATION COMPLETE\n");
  printf("=======================================\n");
  printf("Simulation Time:			%d\n", simTime);
  printf("Maximum Thread Sleep Time:		%d\n", maxSleepTime);
  printf("Number of Producer Threads:		%d\n", producerThreads);
  printf("Number of Consumer Threads:		%d\n", consumerThreads);
  printf("Size of Buffer:		                %d\n\n", BUFFER_SIZE);

  printf("Total Number of Items Produced:		%d\n", totalProduced);
  for (int i = 0; i < producerThreads; i++) {
    printf("  Thread %d:				%d\n", i, producePerThread[i]);
  }

  printf("\n\n");

  printf("Total Number of Items Consumed:		%d\n", totalConsumed);
  for (int i = 0; i < consumerThreads; i++) {
    printf("  Thread %d:				%d\n", i, consumePerThread[i]);
  }

  printf("\n\n");

  printf("Number Of Items Remaining in Buffer:    %d\n", get_buffers_occupied());
  printf("Number Of Times Buffer Was Full:	%d\n", timesFull);
  printf("Number Of Times Buffer Was Empty:	%d\n", timesEmpty);
}

//********************************************************************
//
// Producer Runner
//
// This function is the runner for the producer threads that creates an indefinite while loop
// with a parameter that contains the current thread id. This function will sleep for a random amount of time
// based on the maxSleepTime variable and then creates a random number betwen 0 and 99 and stores it in the buffer if a slot
// in the buffer is avaliable and the mutex lock is free.
//
// Reference Parameters
// --------------------
// param     int             The thread index that was made at the creation of the thread.
//
// Local Variables
// ------------
// int tid              An integer containing the thread index id that was made at the creation of the thread.
// int timeToSleep      An integer that is a random number between 0 and the maxSleepTime global variable which is used for sleeping the thread.
// int nextProduced     An integer generated between 0 and 99 that is stored in the buffer.
//*******************************************************************

void *producer(void *param)
{
  while(1 && !exitFlag) 
  {
    int tid = (int)(intptr_t)param;

    int timeToSleep = rand() % maxSleepTime;
    sleep(timeToSleep);

    int nextProduced = rand() % 100;

    /* If the buffer is full, increment timesFull for simulation stats.*/
    if (is_buffer_full()) {
      if (showBufferStats) {
        printf("All buffers full. Producer is waiting.\n\n");
      }
      timesFull++;
    }

    sem_wait(&empty);
    pthread_mutex_lock(&mutex);
    
    /* Critical Section */

    buffer_insert_item(tid, nextProduced);

    if (showBufferStats) {
      printf("Producer %d writes %d\n", tid, nextProduced);

      /* Print the current buffer */
      print_buffer();
    }

    pthread_mutex_unlock( &mutex );
    sem_post(&full);
  }

  pthread_exit(0);
}

//********************************************************************
//
// Consumer Runner
//
// This function is the runner for the consumer threads that creates an indefinite while loop
// with a parameter that contains the current thread id. This function will sleep for a random amount of time
// based on the maxSleepTime variable and then take a value out of the buffer and read it, determine if the value is prime and consume it if the buffer is currently full
// and the mutex lock is free.
//
// Reference Parameters
// --------------------
// param     int             The thread index id that was made at the creation of the thread.
//
// Local Variables
// ------------
// int tid              An integer containing the thread index id that was made at the creation of the thread.
// int timeToSleep      An integer that is a random number between 0 and the maxSleepTime global variable which is used for sleeping the thread.
// int nextConsumed     An integer that is read and consumed from the buffer
//*******************************************************************

void *consumer(void *param)
{
  while(1 && !exitFlag)
  {
    int tid = (int)(intptr_t)param;

    int timeToSleep = rand() % maxSleepTime;
    sleep(timeToSleep);

    if (is_buffer_empty()) {
      if (showBufferStats) {
         printf("All buffers empty. Consumer is waiting.\n\n");
      }

      timesEmpty++;
    }

    sem_wait(&full);
    pthread_mutex_lock(&mutex);

    int nextConsumed = buffer[out];

    buffer_remove_item(tid, nextConsumed);

    if (showBufferStats) {
      // Check if the nextConsumed number is prime.
      if (nextConsumed % 2 == 0 || nextConsumed % 3 == 0) {
        printf("Consumer %d reads %d\n", tid, nextConsumed);
      } else {
        printf("Consumer %d reads %d    * * * PRIME * * *\n", tid, nextConsumed);
      }

      /* Print the current buffer */
      print_buffer();
    }

    pthread_mutex_unlock(&mutex);
    sem_post(&empty);
  }

  pthread_exit(0);
}

//********************************************************************
//
// Main function
//
// This function is the main body of the process and handles obtaining all of the command line parameters, creating the semaphore/mutex locks,
// creating the producer and consumer threads based on the input amount, then sleeping for the specified simulatiom time
// and afterwards joining all of the threads together and printing the simulation's stats.
//
// Value Parameters
// ----------------
// argc		int		            The amount of parameters passed to the process.
//
// Reference Parameters
// --------------------
// argv     char             An array of all of the passed parameters to the process.
//
// Local Variables
// ------------
// int simTime                An integer containing the duration that the simulation should run before exiting the program.
// int numProducerThreads     An integer that is the number of Producer threads that will be created.
// int numConsumerThreads     An integer that is the number of Consumer threads that will be created.
// pthread_t pNPThreads       A datatype that contains all of the identifiers for the created Producer threads.
// pthread_t pNCThreads       A datatype that contains all of the identifiers for the created Consumer threads.
// pthread_attr_t attr        A struct that is used to determine each of the threads attributes at creation.
//*******************************************************************

int main(int argc, char *argv[])
{
  if (argc < 6) {
    printf("Simulation usage: <simTime> <maxSleepTime> <numProducers> <numConsumers> <showIndividualBufferStats>\n");
    exit(1);
  }

  int simTime = atoi(argv[1]);
  maxSleepTime = atoi(argv[2]);

  int numProducerThreads = atoi(argv[3]);
  int numConsumerThreads = atoi(argv[4]);

  /* Enable individual buffer snapshots if the last argument is "yes" */
  if (strcmp(argv[5], "yes") == 0) {
    showBufferStats = 1;
  }

  /* local variables */
  pthread_t pNPThreads[numProducerThreads], pNCThreads[numConsumerThreads];
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_mutex_init(&mutex, NULL); /* create the mutex lock */
  sem_init( &empty, 0, 5 );
  sem_init( &full, 0, 0 );

  printf("Starting threads...\n");

  /* Show the initial buffer and the number of occupied threads at the start of the simulation. */
  if (showBufferStats) {
    print_buffer();
  }

  for (int i = 0; i < numProducerThreads; i++)
  {
    pthread_create(&pNPThreads[i], &attr, producer, (void*)(intptr_t)i);
  }

  for (int i = 0; i < numConsumerThreads; i++)
  {
    pthread_create(&pNCThreads[i], &attr, consumer, (void*)(intptr_t)i);
  }

  sleep(simTime);

  /* Signal all threads to end their while loop and exit. */
  exitFlag = 1;

  for (int i = 0; i < numProducerThreads; i++)
  {
    pthread_join(pNPThreads[i], NULL);
  }

  for (int i = 0; i < numConsumerThreads; i++)
  {
    pthread_join(pNCThreads[i], NULL);
  }

  sem_destroy(&empty);
  sem_destroy(&full);
  pthread_mutex_destroy(&mutex);

  /* Simulation complete */
  print_simulation_stats(simTime, numProducerThreads, numConsumerThreads);

  return 0;
}