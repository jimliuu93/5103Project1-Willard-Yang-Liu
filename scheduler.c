#include <sys/time.h>
#include "queue.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// Data structures needed for uthread
QUEUE* readyList;
QUEUE* waitList;
QUEUE* finishiedList;
#define ready 0;
#define wait 1;
#define running 2;

// Globals
struct itimerval timer;
uthread* runningThread;

// Helper functions
void thread_switch(uthread* runningThread, uthread* newThread);

// pthread equivalents
int uthread_create(void *(*start_routine)(void*), void *arg);

int uthread_yield(void) {
  uthread *chosenTCB, *finishedTCB;
  // Cannot disable interrupt in user level so ignore disableinterrupt
  NODE* firstNode = Dequeue(readyList);
  chosenTCB = firstNode->TCB;
  if (chosenTCB == NULL) {
    // Nothing els to run, so go back to running original thread.
  } else {
    // Move running thread onto the ready list
    runningThread->state = ready;
    Enqueue(readyList, runningThread);
    thread_switch(runningThread, chosenTCB); //Switch to new thread
    runningThread->state = running;
  }

  // Delete any threads on the finishing list.
  NODE* finishedNode;
  while ((finishedNode = Dequeue(finishiedList)) != NULL) {
    finishedTCB = finishedNode->TCB;
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

// When the time slice, finish, remove the current thread to ready list
void timer_handler (int signum) {
  uthread_yield();
}

int uthread_self(void);
int uthread_join(int tid, void **retval) {
  // Remove the thread from waiting list and put onto ready list
}

// uthread control
int uthread_init(int time_slice) {
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

int uthread_terminate(int tid) {
  // Put the list onto the finished listp
}

int uthread_suspend(int tid) {
  if (runningThread->tid == tid) {
    // Similar to yield
    uthread *chosenTCB, *finishedTCB;
    // Cannot disable interrupt in user level so ignore disableinterrupt
    NODE* readyNode;
    readyNode = Dequeue(readyList);
    chosenTCB = readyNode->TCB;
    if (chosenTCB == NULL) {
      // Nothing else to run, so go back to the running original thread
    } else {
      // If we keep a running list, then we need to loop through the running list
      // and find out the thread we want and store it as runningThread.
      // This step is ignored now and assume we know which is the running thread.
      runningThread->state = ready;
      Enqueue(waitList, runningThread);
      thread_switch(runningThread, chosenTCB); //Switch to new thread
      runningThread->state = running;
    }

    // Delete any threads on the finishing list.
    NODE* finishedNode;
    while ((finishedNode = Dequeue(finishiedList)) != NULL) {
      finishedTCB = finishedNode->TCB;
      free(finishedTCB->stack);
      free(finishedTCB);
    }

    // Set timmer
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
      perror("error calling setitimer");
      exit(1);
    }
  } else {
    // Search in the readyList
    NODE* current = readyList->head;
    uthread *suspended;
    if (tid == current->TCB->tid) {
      suspended = current->TCB;
      readyList->head = current->nextNode;
    } else {
      while (current->nextNode != NULL) {
        if (tid == current->nextNode->TCB->tid) {
          suspended = current->nextNode->TCB;
          current->nextNode = current->nextNode->nextNode;
          break;
        }
        current = current->nextNode;
      }
    }
    Enqueue(waitList, suspended);
  }
  return 0;
}

int uthread_resume(int tid) {
  // Remove the thread from ready list and put onto running list
  NODE* current = readyList->head;
  uthread *nextRun, *finishiedTCB;
  if (tid == current->TCB->tid) {
    nextRun = current->TCB;
    readyList->head = current->nextNode;
  } else {
    while (current->nextNode != NULL) {
      if (tid == current->nextNode->TCB->tid) {
        nextRun = current->nextNode->TCB;
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
  runningThread->state = ready;
  Enqueue(readyList, runningThread);
  thread_switch(runningThread, nextRun);
  runningThread->state = running;

  // Delete any threads on the finishing list.
  NODE* finishedNode;
  while ((finishedNode = Dequeue(finishiedList)) != NULL) {
    finishiedTCB = finishedNode->TCB;
    free(finishiedTCB->stack);
    free(finishiedTCB);
  }
  // Set timmer

  if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
    perror("error calling setitimer");
    exit(1);
  }
  return 0;
}

// Asynchronous I/O
ssize_t async_read(int fildes, void *buf, size_t nbytes);
