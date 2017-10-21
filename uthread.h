#include <ucontext.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef H_UTHREAD
#define H_UTHREAD

static int STACK_SIZE = 8192;

typedef unsigned long address_t;
//thread states
enum uthread_state {
	ST_INIT,
	ST_READY,
	ST_RUNNING,
	ST_WAITING,
	ST_FINISHED
};


//tcb type
typedef struct {
	int tid;
	void* sp;
	void* pc;
	char* stack;
	void* retVal;
	int stack_size;
	ucontext_t* context;
	enum uthread_state state;	//state the thread is in
	void* (*start_routine)(void *); //func pointer to the thread function to be executed
	void *args;			// arguments to thread function
} tcb_t;

//QUEUE--------------------------------------------------------------------------------------------
typedef struct node_t node_t;
struct node_t{
    tcb_t *tcb; // Thread control block
    node_t *nextNode;
};

//node constructor
node_t* node_t_new(tcb_t *n_tcb){
	node_t* newNode = (node_t *)malloc(sizeof(node_t));
	newNode->tcb = n_tcb;
}


typedef struct {
    node_t *head;
    node_t *tail;

} queue_t;

queue_t * InitQueue(void) {
    queue_t *myQueue = (queue_t *)malloc(sizeof(queue_t));

    myQueue->head = NULL;
    myQueue->tail = NULL;

    return myQueue;
}

node_t * Dequeue(queue_t *queue) {
    node_t *pNode = queue->head;
    if(pNode != NULL){
       queue->head = pNode->nextNode;
    }
    return pNode;
}

void Enqueue(queue_t *queue, tcb_t *tcb) {
//    printf("Enqueue called\n");
    node_t *newNode = (node_t *)malloc(sizeof(node_t));
    //node_t *newNode = node_t_new(tcb);
 //   printf("new node created for tid=%d\n", tcb->tid);
    newNode->tcb = tcb;
    newNode->nextNode = NULL;
  //  printf("newNode given tcb struct member\n");
    if(queue->tail == NULL){
			queue->tail = newNode;
			queue->head = newNode;
    }
    //queue->tail->nextNode = (node_t *)malloc(sizeof(node_t));
    else {
      queue->tail->nextNode = newNode;
   //   printf("tail nextnode set to new node\n");
      queue->tail = newNode;
    //  printf("queue tail updated to contain new node\n");
    }
    return;
}

tcb_t *FindTCB_ById(queue_t *queue, int tid){
	node_t * node = queue->head;
	if(queue->head != NULL){
		while(node != NULL){
			if(node->tcb->tid == tid){
				//break if node is found
				return node->tcb;
			}
			if(node == queue->tail){
				//break if node is not in queue
				break;
			}
			node = node->nextNode;
		}
	}
	return NULL;
}
//-----------------------------------------------------------------------------------------------


//thread type
typedef struct {
	tcb_t* tcb;
	int joinFrom_tid;
} uthread_t;

void uthread_init(int time_slice);
int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg);
int uthread_yield(void);
int uthread_self(void);
int uthread_join(int tid, void **retval);
void uthread_switch(tcb_t * oldTcb, tcb_t * newTcb);
int uthread_suspend(int tid);
#endif
