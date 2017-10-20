#include <ucontext.h>
#include <malloc.h>

#ifndef H_UTHREAD
#define H_UTHREAD


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
	int stackSize;
	ucontext_t* context;	
	enum uthread_state state;	//state the thread is in
	void* (*start_routine)(void *); //func pointer to the thread function to be executed
	void *args;			// arguments to thread function
} tcb_t;

//QUEUE
typedef struct node_t node_t;
struct node_t{                                                                                   
    tcb_t tcb; // Thread control block
    node_t *nextNode;
    node_t(tcb_t n_tcb){
	tcb = n_tcb; 
    }
} ;

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

    queue->head = pNode->nextNode;

    return pNode;
}

void Enqueue(queue_t *queue, tcb_t tcb) {
    node_t *newNode = (node_t *)malloc(sizeof(node_t));

    newNode->tcb = tcb;
    newNode->nextNode = NULL;

    queue->tail->nextNode = newNode;
    queue->tail = newNode;

    return;
}


//thread type
typedef struct {
	tcb_t* tcb;
	queue_t* waitThreadList;
	int joinFrom_tid;
} uthread_t;

int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg);
int uthread_yield(void);
int uthread_self(void);
int uthread_join(int tid, void **retval);

#endif
