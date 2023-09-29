////// Header Files /////

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <syslog.h>


///// Global Variables /////

#define COUNT  128

typedef struct
{
    int threadIdx;
} threadParams_t;


////////// Syslog Variables ///////////

char buffer[1024];

///// POSIX thread declarations and scheduling attributes /////

pthread_t threads[COUNT];
threadParams_t threadParams[COUNT];

int gsum=0;


////// Entry point function for threads //////

void *adder(void *threadp)
{
    int i;
    gsum = 0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i = 0; i < (threadParams-> threadIdx); i++)
        gsum=gsum+i;

    syslog(LOG_INFO, "Thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
}


int main (int argc, char *argv[])
{

   int i=0;


   ////// Syslog Initialization /////

   openlog("The sum for", LOG_CONS, LOG_USER);


   ///// Thread Creation and Joining /////

   for(i = 0; i < COUNT; i++){

   	threadParams[i].threadIdx=i;
   	pthread_create(&threads[i],   				// pointer to thread descriptor
                  	(void *)0,     				// use default attributes
                  	adder, 					// thread function entry point
                  	(void *)&(threadParams[i]) 		// parameters to pass in
                 	);

   }

   for(i=0; i < COUNT; i++)
     pthread_join(threads[i], NULL);

   closelog();
}
