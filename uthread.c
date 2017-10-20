#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "uthread.h"
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <ucontext.h>

typedef unsigned long address_t;

uthread_t * runningThread = NULL;
queue_t* readyQueue = NULL;
queue_t* waitingQueue = NULL;
queue_t* finishedQueue = NULL; 

//keep track for threadId
static int numThreads = 0;
static int INITIAL_STACK_SIZE = 8192;

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
	if(waitingQueue == NULL){
		waitingQueue = (queue_t *) malloc(sizeof(queue_t));
	}
	//set interrupt timer
	//timer.it_interval.tv_sec = time_slice/1000000;
	//timer.it_interval.tv_usec = time_slice%1000000;
	//timer.it_value = timer.it_interval;	

	//initialize main thread
	//uthread_t *mainThread = (uthread_t*) malloc(sizeof(uthread_t));
	//tcb_t * tcb = (tcb_t*)malloc(sizeof(tcb_t));
//	tcb->tid = numThreads++;
	//tcb->state = ST_READY;
	//mainThread->tcb = tcb;
	//if(getcontext(&main_context) == -1){
	//	perror("get context failed");
	//	exit(EXIT_FAILURE);
	//}
 
	//mainThread->tcb->context = &main_context;
	//Enqueue(readyQueue, *(mainThread->tcb));	 
	//setitimer(ITIMER_PROF, &timer, NULL); 
}
void uthread_exit(void* retval){
	//untested
	tcb_t * currentTCB = runningThread->tcb;
	currentTCB->args = retval;
	currentTCB->state = ST_FINISHED;
	Enqueue(finishedQueue, *currentTCB);
}
void stub(void (*func)(int), int arg){
	(*func)(arg);
	uthread_exit(0);
}

int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg){
	if(readyQueue != NULL){
		uthread_t *newThread = (uthread_t*) malloc(sizeof(uthread_t));
		tcb_t * tcb = (tcb_t*)malloc(sizeof(tcb_t));
		//initialize ucontext
		tcb->context = (ucontext_t*)malloc(sizeof(ucontext_t));
		getcontext(tcb->context);
		//get threadId
		tcb->tid = numThreads++;
		//initialize stack and staack size
		tcb->stack_size = INITIAL_STACK_SIZE;
		tcb->stack = (char *)malloc(INITIAL_STACK_SIZE);
		//set stack pointer and program counter
		tcb->sp = tcb->stack + INITIAL_STACK_SIZE; //minus size of int?
		tcb->pc = &stub;
		//create a stack frame by pushing stub's arguments and start address onto the stack
		//*((typeof(arg)*)tcb->sp) = arg; //don't need this if storing in TCB?
		//tcb->sp--;
		*((typeof(start_routine)*)tcb->sp) = (*start_routine);
		tcb->sp--;
		//store arguments to thread function
		tcb->args = arg;
		newThread->tcb = tcb; 
		tcb->state = ST_READY;
		Enqueue(readyQueue, (*tcb));
		printf("Thread created with threadId=%d, put into ready queue", tcb->tid);
	}
	else {
		perror("readyQueue not initalized for thread creation");
		exit(EXIT_FAILURE);
	}
}

int uthread_yield(){
	tcb_t *nextTcb, *finishedTcb;
	//disable interrupts	

	//choose another TCB from the ready queue
	nextTcb = (tcb_t *)Dequeue(readyQueue);	
	if (nextTcb == NULL){
		
	} else {
		//move running thread onto ready queue
		runningThread->tcb->state = ST_READY;
		Enqueue(readyQueue, *(runningThread->tcb));
		//call thread switch
		uthread_switch(runningThread->tcb, nextTcb);
		// here

		runningThread->tcb->state = ST_RUNNING;
	}
	//DELETE threads on finished list?	
	return 0;	
}

int uthread_self(void){
	//should return calling thread
	//return ((tcb_t *) queue_t(readyQueue))->tid;
	return 0;
}

void uthread_switch(tcb_t * oldThreadTCB, tcb_t * newThreadTCB){
	//set the context of the oldThread to the current context
	getcontext(oldThreadTCB->context);
	//save the currently running threads registers to TCB and stack
	oldThreadTCB->context->uc_stack.ss_sp = oldThreadTCB->sp;
	//switch to new thread context
	setcontext(newThreadTCB->context);
}




int uthread_join(int tid, void** retVal){
	//check if thread has finished	
	tcb_t * foundTcb = FindTCB_ById(finishedQueue, tid);
	
	//waits for termination of specified thread
	tcb_t * threadToDie = (tcb_t*)malloc(sizeof(tcb_t));
	while(threadToDie->tid != tid){
		tcb_t * foundTcb = FindTCB_ById(finishedQueue, tid);
		if(foundTcb->tid == tid){
			printf("after waiting, thread %d has finished\n", foundTcb->tid);
			threadToDie = foundTcb;	
		}
		sleep(0.2);	
	}	

}
