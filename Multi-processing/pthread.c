///// Header Files //////

#define _GNU_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/sysinfo.h>


///// Global Variables ///////////////

char buffer[1024];
#define NUM_THREADS (128)
#define NUM_CPUS (4)

typedef struct
{
    int threadIdx;
} threadParams_t;


///// POSIX thread declarations and scheduling attributes /////

pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;
pthread_attr_t main_attr;
pid_t mainpid;


int gsum=0;



///// Entry point function for threads /////

void *adder(void *threadp)
{

    int i;
    gsum = 0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=1; i < (threadParams->threadIdx)+1; i++)
        gsum=gsum+i;


   syslog(LOG_INFO, "Thread i=%d, sum[1...%d]=%d running on core: %d\n", threadParams->threadIdx, threadParams->threadIdx, gsum, sched_getcpu());


}



///// Main function /////

int main (int argc, char *argv[])
{
   int i, rc;


   ////// Syslog Initialization /////


   openlog("", LOG_CONS, LOG_USER);


   ///// Scheduler Initialization /////

   mainpid=getpid();

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   rc=sched_getparam(mainpid, &main_param);
   main_param.sched_priority=rt_max_prio;

   if(rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param) < 0)
           perror("******** WARNING: sched_setscheduler");


   ///// Thread Creation /////

   for(i = 1; i <= NUM_THREADS; i++){

	rc = pthread_attr_init(&rt_sched_attr[i]);
	rc = pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
	rc = pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);

	rt_param[i].sched_priority = rt_max_prio - i - 1;
	pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

   	threadParams[i].threadIdx=i;

   	pthread_create(&threads[i],   // pointer to thread descriptor
                  	&rt_sched_attr[i],     // use default attributes
                  	adder, // thread function entry point
                  	(void *)&(threadParams[i]) // parameters to pass in
                );

   }

   ///// Thread Joining /////

   for(i=1; i <= NUM_THREADS; i++)
     pthread_join(threads[i], NULL);

   closelog();
}
