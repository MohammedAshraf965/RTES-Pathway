////// Header Files //////

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sched.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include <syslog.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <errno.h>

#define NUM_THREADS (4)
//#define NUM_CPU_CORES (4)

#define TRUE (1)
#define FALSE (0)

///// Global Variables /////

char buffer[1024];

typedef struct{
	int threadIdx;
	unsigned long long sequencePeriods;
}threadParams_t;

sem_t semS1, semS2, semS3;

int abortTest = FALSE;
int abortS1 = FALSE, abortS2 = FALSE, abortS3 = FALSE;

struct timespec start_time_val;
double start_realtime;

///// Sequencer and Services /////

void *Sequencer(void *threadp);

void *Service_1(void *threadp);
void *Service_2(void *threadp);
void *Service_3(void *threadp);

///// Realtime function /////

double realtime(struct timespec *tsptr);

///// Fib Test /////

unsigned int idx = 0, jdx = 1;
unsigned int seqIterations = 47, reqIterations = 1000000;
volatile signed int fib = 0, fib0 = 0, fib1 = 1;

int FIB_TEST(unsigned int seqint, unsigned int iterCnt);

///// Main Function /////

int main(int argc, char *argv[]){

	////// Syslog Initialization //////

	char cmd_command[20] = "uname -a";

	FILE* uname_output = popen(cmd_command, "r");
	fgets(buffer, sizeof(buffer), uname_output);
	pclose(uname_output);

	openlog("[COURSE:2][ASSIGNMENT:1]", LOG_CONS, LOG_USER);
	syslog(LOG_INFO, "%s", buffer);

	///// Local Variables /////

	int i, rc;

	pthread_t threads[NUM_THREADS];
	pthread_attr_t main_attr, rt_sched_attr[NUM_THREADS];
	struct sched_param main_param, rt_param[NUM_THREADS];
	int rt_max_prio, rt_min_prio;
	pid_t mainpid;
	threadParams_t threadParams[NUM_THREADS];

	cpu_set_t threadcpu, allcpuset;
	int cpuidx;

	//CPU_ZERO(&allcpuset);

	//for(i=0; i<NUM_CPU_CORES; i++)
	//	CPU_SET(i, &allcpuset);

	mainpid = getpid();

	rt_max_prio = sched_get_priority_max(SCHED_FIFO);
	rt_min_prio = sched_get_priority_min(SCHED_FIFO);

	rc = sched_getparam(mainpid, &main_param);
	main_param.sched_priority = rt_max_prio;
	rc = sched_setscheduler(getpid(), SCHED_FIFO, &main_param);

	if(rc < 0)
		perror("Main param");

	//syslog(LOG_INFO, "%d", &threadParams);
	//syslog(LOG_INFO, "%d", rc);

	for(i=0; i < NUM_THREADS; i++){

		rc = pthread_attr_init(&rt_sched_attr[i]);
		rc = pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
		rc = pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);

		rt_param[i].sched_priority = rt_max_prio - i;

		pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

		threadParams[i].threadIdx = i;
	}

	////// Service Threads Creaation //////

	rt_param[1].sched_priority = rt_max_prio - 1;
	pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);

	pthread_create(&threads[1],
			&rt_sched_attr[1],
			Service_1,
			(void*)&(threadParams[1])
			);

	if(rc < 0)
		perror("Failed to create a thread for Service_1\n");
	else
		printf("Thread creation for Service_1 was successful\n");

	rt_param[2].sched_priority = rt_max_prio - 2;
	pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);

	pthread_create(&threads[2],
			&rt_sched_attr[2],
			Service_2,
			(void *)&(threadParams[2])
			);

	if(rc < 0)
		perror("Failed to create a thread for Service_2\n");
	else
		printf("Thread creation for Service_2 was successful\n");


	rt_param[3].sched_priority = rt_max_prio - 3;
	pthread_attr_setschedparam(&rt_sched_attr[3], &rt_param[3]);

	pthread_create(&threads[3],
			&rt_sched_attr[3],
			Service_3,
			(void *)&(threadParams[3])
			);

	if(rc < 0)
		perror("Failed to create a thread for Service_3\n");
	else
		printf("Thread creation for Service_3 was successful\n");


	//// Start Sequencer ////

	threadParams[0].sequencePeriods = 2000;

	rt_param[0].sched_priority = rt_max_prio;
	pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);

	pthread_create(&threads[0],
			&rt_sched_attr[0],
			Sequencer,
			(void *)&(threadParams[0])
			);

	if(rc < 0)
		perror("Failed to create a thread for Sequencer\n");
	else
		printf("Thread creation for Sequencer was successful\n");

	closelog();

}


void *Service_1(void *threadp){

	struct timespec current_time_val;
	double current_realtime;
	unsigned long long seqCnt = 0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	clock_gettime(CLOCK_MONOTONIC_RAW, &current_time_val);
	current_realtime = realtime(&current_time_val);

	syslog(LOG_INFO, "S1 Thread @ sec = %6.9lf\n", current_realtime - start_realtime);

	FIB_TEST(seqIterations, reqIterations);

	while(!abortS1){

		sem_wait(&semS1);
		seqCnt++;

		clock_gettime(CLOCK_MONOTONIC_RAW, &current_time_val);
		current_realtime = realtime(&current_time_val);

		syslog(LOG_INFO, "S1 50 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), seqCnt, current_realtime - start_realtime);

	}

	pthread_exit((void *)0);
}

void *Service_2(void *threadp){

	struct timespec current_time_val;
	double current_realtime;
	unsigned long long seqCnt = 0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	clock_gettime(CLOCK_MONOTONIC_RAW, &current_time_val);
	current_realtime = realtime(&current_time_val);

	syslog(LOG_INFO, "S2 Thread @ sec %6.9lf\n", current_realtime - start_realtime);

	FIB_TEST(seqIterations, reqIterations);

	while(!abortS2){

		sem_wait(&semS1);
		seqCnt++;

		clock_gettime(CLOCK_MONOTONIC_RAW, &current_time_val);
		current_realtime = realtime(&current_time_val);

		syslog(LOG_INFO, "S2 100 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), seqCnt, current_realtime - start_realtime);

	}

	pthread_exit((void *)0);

}


void *Service_3(void *threadp){

	struct timespec current_time_val;
	double current_realtime;
	unsigned long long seqCnt = 0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	clock_gettime(CLOCK_MONOTONIC_RAW, &current_time_val);
	current_realtime = realtime(&current_time_val);

	syslog(LOG_INFO, "S3 Thread @ sec = %6.9lf\n", current_realtime - start_realtime);

	FIB_TEST(seqIterations, reqIterations);

	while(!abortS3){

		sem_wait(&semS3);
		seqCnt++;

		clock_gettime(CLOCK_MONOTONIC_RAW, &current_time_val);
		current_realtime = realtime(&current_time_val);

		syslog(LOG_INFO, "S3 120 Hz on core %d for release %llu @ sec = %6.9lf\n", sched_getcpu(), seqCnt, current_realtime - start_realtime);

	}

	pthread_exit((void *)0);

}

void *Sequencer(void *threadp){

	struct timespec current_time_val;
	struct timespec delay_time = {0,10000000};
	struct timespec remaining_time;
	double current_realtime;
	int rc;
	unsigned long long seqCnt = 0;
	threadParams_t *threadParams = (threadParams_t *)threadp;

	clock_gettime(CLOCK_MONOTONIC_RAW, &current_time_val);
	current_realtime = realtime(&current_time_val);

	syslog(LOG_INFO, " Sequencer thread @ sec = %6.9lf\n", current_realtime);

	do{

		if(rc = nanosleep(&delay_time, &remaining_time) == 0)
			break;

		seqCnt++;

		current_realtime = current_realtime + ((double)delay_time.tv_nsec / 1000000000); // Calling time in user kernel space results in delta-T in time

		syslog(LOG_INFO, "Sequencer on core %d for cycle %llu @ sec = 6.9lf\n", sched_getcpu(), seqCnt, current_realtime - start_realtime);


		// Service_1
		if(seqCnt % 2 == 0)
			sem_post(&semS1);

		// Service 2
		if(seqCnt % 10 == 0)
			sem_post(&semS2);

		//Service_3
		if(seqCnt % 15 == 0)
			sem_post(&semS3);

	}

	while(!abortTest && (seqCnt < threadParams -> sequencePeriods));

	sem_post(&semS1); sem_post(&semS2); sem_post(&semS3);

	abortS1 = TRUE;	abortS2 = TRUE; abortS3 = TRUE;

	pthread_exit((void *)0);

}


double realtime(struct timespec *tsptr){

	return( (double)(tsptr -> tv_sec) + (((double)tsptr -> tv_sec)/1000000000.0));
}


int FIB_TEST(unsigned int seqCnt, unsigned int iterCnt){

	for(idx = 0; idx < iterCnt; idx++){

		fib = fib0 + fib1;

		while(jdx < seqCnt){

			fib0 = fib1;
			fib1 = fib;
			fib = fib0 + fib1;
			jdx++;
		}

		jdx = 0;
	}

	return idx;
}
