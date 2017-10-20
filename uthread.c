#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "uthread.h"
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <ucontext.h>

typedef unsigned long address_t;

tcb_t * runningThread = NULL;
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

void uthread_exit(void* retval){
	//untested
	tcb_t * currentTCB = runningThread;
	currentTCB->args = retval;
	currentTCB->state = ST_FINISHED;
	Enqueue(finishedQueue, currentTCB);
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
		//store arguments to thread function in tcb
		tcb->args = arg;
		newThread->tcb = tcb;
		tcb->state = ST_READY;
		Enqueue(readyQueue, tcb);
		printf("Thread created with threadId=%d, put into ready queue", tcb->tid);
	}
	else {
		perror("readyQueue not initalized for thread creation");
		exit(EXIT_FAILURE);
	}
}

int uthread_yield(void) {
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGVTALRM);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);

  tcb_t *chosenTCB, *finishedTCB;
  // Cannot disable interrupt in user level so ignore disableinterrupt
  node_t* firstNode = Dequeue(readyQueue);
  chosenTCB = firstNode->tcb;
  if (chosenTCB == NULL) {
    // Nothing else to run, so go back to running original thread.
  } else {
    // Move running thread onto the ready list
    runningThread->state = ST_READY;
    Enqueue(readyQueue, runningThread);
    uthread_switch(runningThread, chosenTCB); //Switch to new thread
    runningThread->state = ST_RUNNING;
  }

  // Delete any threads on the finishing list.
  node_t* finishedNode;
  while ((finishedNode = Dequeue(finishedQueue)) != NULL) {
    finishedTCB = finishedNode->tcb;
    free(finishedTCB->stack);
    free(finishedTCB);
  }

  // Set timmer
  if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
    perror("error calling setitimer");
    exit(1);
  }
  sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
  return 0;
}

// When the time slice, finish, remove the current thread to ready list
void timer_handler (int signum) {
  uthread_yield();
}

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
  	timer.it_value.tv_sec = 0;
  	timer.it_value.tv_usec = time_slice;
  	timer.it_interval.tv_sec = 0;
  	timer.it_interval.tv_usec = 0;
  	// Part of the init to set up alarm handler
  	struct sigaction sa;
  	/* Install timer_handler as the signal handler for SIGVTALRM. */
  	memset (&sa, 0, sizeof (sa));
  	sa.sa_handler = &timer_handler;
  	sigaction (SIGVTALRM, &sa, NULL);
}

int uthread_self(void){
	//should return calling thread
	//return ((tcb_t *) queue_t(readyQueue))->tid;
	return runningThread->tid;
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
	if(FindTCB_ById(waitingQueue, tid) != NULL || FindTCB_ById(readyQueue, tid) != NULL){
		while(threadToDie->tid != tid){
			tcb_t * foundTcb = FindTCB_ById(finishedQueue, tid);
			if(foundTcb->tid == tid){
				printf("after waiting, thread %d has finished\n", foundTcb->tid);
				threadToDie = foundTcb;
			}
			//if(foundTcb == finishedQueue
			sleep(0.2);
			printf("waiting for thread %d\n", tid);
		}
	}
	else{
		perror("Tried to join with nonexistent thread\n");
		exit(1);
	}
}

int uthread_suspend(int tid) {
  if (runningThread->tid == tid) {
    // Similar to yield
    tcb_t *chosenTCB, *finishedTCB;
    // Cannot disable interrupt in user level so ignore disableinterrupt
    node_t* readyNode;
    readyNode = Dequeue(readyQueue);
    chosenTCB = readyNode->tcb;
    if (chosenTCB == NULL) {
      // Nothing else to run, so go back to the running original thread
    } else {
      // If we keep a running list, then we need to loop through the running list
      // and find out the thread we want and store it as runningThread.
      // This step is ignored now and assume we know which is the running thread.
      runningThread->state = ST_WAITING;
      Enqueue(waitingQueue, runningThread);
      uthread_switch(runningThread, chosenTCB); //Switch to new thread
      runningThread->state = ST_RUNNING;
    }

    // Delete any threads on the finishing list.
    node_t* finishedNode;
    while ((finishedNode = Dequeue(finishedQueue)) != NULL) {
      finishedTCB = finishedNode->tcb;
      free(finishedTCB->stack);
      free(finishedTCB);
    }

    // Set timmer
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
      perror("error calling setitimer");
      exit(1);
    }
  } else {
    // Search in the readyQueue
    node_t* current = readyQueue->head;
    tcb_t *suspended;
    if (tid == current->tcb->tid) {
      suspended = current->tcb;
      readyQueue->head = current->nextNode;
    } else {
      while (current->nextNode != NULL) {
        if (tid == current->nextNode->tcb->tid) {
          suspended = current->nextNode->tcb;
          current->nextNode = current->nextNode->nextNode;
          break;
        }
        current = current->nextNode;
      }
    }
    suspended->state = ST_WAITING;
    Enqueue(waitingQueue, suspended);
  }
  return 0;
}

int uthread_resume(int tid) {
  // Remove the thread from ready list and put onto running list
  node_t* current = readyQueue->head;
  tcb_t *nextRun, *finishedTCB;
  if (tid == current->tcb->tid) {
    nextRun = current->tcb;
    readyQueue->head = current->nextNode;
  } else {
    while (current->nextNode != NULL) {
      if (tid == current->nextNode->tcb->tid) {
        nextRun = current->nextNode->tcb;
        current->nextNode = current->nextNode->nextNode;
        break;
      }
      current = current->nextNode;
    }
    // If failed to find the TCB of given tid, throw error
    if (nextRun == NULL) {
      printf("Fail to find the thread of given tid in the ready list\n");
    }
  }
  runningThread->state = ST_READY;
  Enqueue(readyQueue, runningThread);
  uthread_switch(runningThread, nextRun);
  runningThread->state = ST_RUNNING;

  // Delete any threads on the finishing list.
  node_t* finishedNode;
  while ((finishedNode = Dequeue(finishedQueue)) != NULL) {
    finishedTCB = finishedNode->tcb;
    free(finishedTCB->stack);
    free(finishedTCB);
  }
  // Set timmer

  if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
    perror("error calling setitimer");
    exit(1);
  }
  return 0;
}

int uthread_terminate(int tid) {
  // Terminate a thread and move to finishied list.
  // Check whether thread being terminated is the running thread.
  if (runningThread->tid == tid) {
    uthread_exit(0);
    uthread_yield();
  }
  // Check whether the tread being terminated is in the ready list.
  // It is possile that it is originally in the ready list or it was the
  // running thread and just added to the ready list.
  node_t* current = readyQueue->head;
  tcb_t *finishied;
  int inWaitList = 0;
  if (tid == current->tcb->tid) {
    finishied = current->tcb;
    readyQueue->head = current->nextNode;
    inWaitList = 1;
  } else {
    while (current->nextNode != NULL) {
      if (tid == current->nextNode->tcb->tid) {
        finishied = current->nextNode->tcb;
        current->nextNode = current->nextNode->nextNode;
        inWaitList = 1;
        break;
      }
      current = current->nextNode;
    }
  }
  if (inWaitList) {
    finishied->state = ST_FINISHED;
    Enqueue(finishedQueue, finishied);
  } else  {
    // If the thread is neither the running thread nor in ready list, it should
    // be in the wait list. So search wait list.
    current = waitingQueue->head;
    if (tid == current->tcb->tid) {
      finishied = current->tcb;
      waitingQueue->head = current->nextNode;
    } else {
      while (current->nextNode != NULL) {
        if (tid == current->nextNode->tcb->tid) {
          finishied = current->nextNode->tcb;
          current->nextNode = current->nextNode->nextNode;
          break;
        }
        current = current->nextNode;
      }
    }
    finishied->state = ST_FINISHED;
    Enqueue(finishedQueue, finishied);
  }

  return 0;
}
