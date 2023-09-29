///// Header Files //////


#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <syslog.h>


///// Global Variables /////

#define NUM_THREADS 1

typedef struct
{
    int threadIdx;
} threadParams_t;



//////// Syslog Variables ///////////

char buffer[1024];
char *thread_message;
char *main_message;

////////////////////////////////////



// POSIX thread declarations and scheduling attributes
//
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];



///// Entry point function for threads /////

void *counterThread(void *threadp)
{
    int sum=0, i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=1; i < (threadParams->threadIdx)+1; i++)
        sum=sum+i;

   // printf("Thread idx=%d, sum[1...%d]=%d\n",
          // threadParams->threadIdx,
          // threadParams->threadIdx, sum);

   thread_message = "Hello World from Thread!";

}


///// Main Function /////

int main (int argc, char *argv[])
{

   //int rc;
   int i;


   ///// Thread Creation /////

   for(i=1; i <= NUM_THREADS; i++)
   {
       threadParams[i].threadIdx=i;

       pthread_create(&threads[i],   // pointer to thread descriptor
                      (void *)0,     // use default attributes
                      counterThread, // thread function entry point
                      (void *)&(threadParams[i]) // parameters to pass in
                     );
   }

   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

  // printf("TEST COMPLETE\n");


   ////// Syslog Initialization ////

   char cmd_command[20] = "uname -a";

   FILE* uname_output = popen(cmd_command, "r");  // Opens a process by creating a pipe, forking, and invoking the shell.
                              			// First argument is a shell command and the second can be -r for Reading or -w for Writing.

   fgets(buffer, sizeof(buffer), uname_output); // Reads the output of uname_output to the buffer according to the size of the buffer.

   pclose(uname_output);                       // Closes the process.


   openlog("[COURSE:1][ASSIGNMENT:1]", LOG_CONS, LOG_USER);
   syslog(LOG_INFO, "%s", buffer);

   main_message = "Hello World from Main!";

   syslog(LOG_INFO, "%s", main_message);
   syslog(LOG_INFO, "%s", thread_message);

   closelog();

   ////////////////////////////////////////

}
