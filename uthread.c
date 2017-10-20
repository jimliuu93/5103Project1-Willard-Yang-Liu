#include <ucontext.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "uthread.h"

typedef void * Thread; 

queue_t* readyQueue = NULL;
queue_t* waitingQueue = NULL;
queue_t* finishedQueue = NULL; 

//keep track for threadId
static int numThreads = 0;

struct itimerval timer;
struct sigaction timerhandler;

sigset_t sigmaskset;
ucontext_t main_context;

void uthread_init(int time_slice){
	//initialize queues
	if(readyQueue == NULL){
		readyQueue = (queue_t *) malloc(sizeof(queue_t));
	}
	if(finishedQueue == NULL){
		finishedQueue = (queue_t *) malloc(sizeof(queue_t));
	}
	//set interrupt timer
	timer.it_interval.tv_sec = time_slice/1000000;
	timer.it_interval.tv_usec = time_slice%1000000;
	timer.it_value = timer.it_interval;	

	//initialize main thread
	uthread_t *mainThread = (uthread_t*) malloc(sizeof(uthread_t));
	tcb_t * tcb = (tcb_t*)malloc(sizeof(tcb_t));
	tcb->tid = numThreads++;
	tcb->state = ST_READY;
	mainThread->tcb = tcb;
	if(getcontext(&main_context) == -1){
		perror("get context failed");
		exit(EXIT_FAILURE);
	}
 
	mainThread->tcb->context = &main_context;
	Enqueue(readyQueue, *(mainThread->tcb));	 
	setitimer(ITIMER_PROF, &timer, NULL); 
}

int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg){
	//sigprocmask(SIG_BLOCK, signalmaskhere, NULL); //block signals to caller thread	
	if(readyQueue != NULL){
		uthread_t *newThread = (uthread_t*) malloc(sizeof(uthread_t));
		tcb_t * tcb = (tcb_t*)malloc(sizeof(tcb_t));
		tcb->tid = numThreads++;
		tcb->state = ST_READY;
		newThread->tcb = tcb; 
	}
	//sigprocmask(SIG_UNBLOCK, signalmaskhere, NULL);
}

int uthread_self(void){
	//should return calling thread
	//return ((tcb_t *) queue_t(readyQueue))->tid;
	return 0;
}

void uthread_exit(void* retval){
	//sig block here
	tcb_t * currentTCB = (tcb_t *) Dequeue(readyQueue);
	currentTCB->args = retval;
	currentTCB->state = ST_FINISHED;
	Enqueue(finishedQueue, *currentTCB);
	//sig unblock here
}

int uthread_join(int tid, void** retVal){
	
}
