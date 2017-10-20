#include <sys/time.h>
#include "uthread.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// Data structures needed for uthread
queue_t* readyList;
queue_t* waitList;
queue_t* finishiedList;

// Globals
struct itimerval timer;
tcb_t* runningThread;

// Helper functions
void thread_switch(tcb_t* runningThread, tcb_t* newThread);

int uthread_yield(void) {
  sigset_t signal_set;

  sigemptyset(&signal_set);

  sigaddset(&signal_set, SIGVTALRM);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);
  tcb_t *chosenTCB, *finishedTCB;
  // Cannot disable interrupt in user level so ignore disableinterrupt
  node_t* firstNode = Dequeue(readyList);
  chosenTCB = firstNode->tcb;
  if (chosenTCB == NULL) {
    // Nothing els to run, so go back to running original thread.
  } else {
    // Move running thread onto the ready list
    runningThread->state = ST_READY;
    Enqueue(readyList, runningThread);
    thread_switch(runningThread, chosenTCB); //Switch to new thread
    runningThread->state = ST_READY;
  }

  // Delete any threads on the finishing list.
  node_t* finishedNode;
  while ((finishedNode = Dequeue(finishiedList)) != NULL) {
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
    tcb_t *chosenTCB, *finishedTCB;
    // Cannot disable interrupt in user level so ignore disableinterrupt
    node_t* readyNode;
    readyNode = Dequeue(readyList);
    chosenTCB = readyNode->tcb;
    if (chosenTCB == NULL) {
      // Nothing else to run, so go back to the running original thread
    } else {
      // If we keep a running list, then we need to loop through the running list
      // and find out the thread we want and store it as runningThread.
      // This step is ignored now and assume we know which is the running thread.
      runningThread->state = ST_READY;
      Enqueue(waitList, runningThread);
      thread_switch(runningThread, chosenTCB); //Switch to new thread
      runningThread->state = ST_READY;
    }

    // Delete any threads on the finishing list.
    node_t* finishedNode;
    while ((finishedNode = Dequeue(finishiedList)) != NULL) {
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
    // Search in the readyList
    node_t* current = readyList->head;
    tcb_t *suspended;
    if (tid == current->tcb->tid) {
      suspended = current->tcb;
      readyList->head = current->nextNode;
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
    Enqueue(waitList, suspended);
  }
  return 0;
}

int uthread_resume(int tid) {
  // Remove the thread from ready list and put onto running list
  node_t* current = readyList->head;
  tcb_t *nextRun, *finishiedTCB;
  if (tid == current->tcb->tid) {
    nextRun = current->tcb;
    readyList->head = current->nextNode;
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
  Enqueue(readyList, runningThread);
  thread_switch(runningThread, nextRun);
  runningThread->state = ST_RUNNING;

  // Delete any threads on the finishing list.
  node_t* finishedNode;
  while ((finishedNode = Dequeue(finishiedList)) != NULL) {
    finishiedTCB = finishedNode->tcb;
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
