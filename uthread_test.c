#include <stdio.h>
#include "uthread.h"
#include "uthread.c"

static void* go(void * n);

#define NTHREADS 10
static uthread_t threads[NTHREADS];

void multiThreadHelloWorld() {
	int i;
	long exitValue;
	void* (*fun_ptr)(void*) = &go;	
	for(i = 0; i < NTHREADS; i++){
		uthread_create(&(threads[i]), fun_ptr, &i);
	}
	for(i = 0; i < NTHREADS; i++){
		void ** retVal = (void **) 0;
		exitValue = uthread_join(threads[i].tcb->tid, retVal);
		printf("Thread %d returned with %ld \n", i, exitValue);
	}
	printf("Main thread done.\n");
}

void* go(void * n) {
	int i = *((int*)n);
	printf("Hello from thread %d\n", i);
	int exitInt = 100 + 1;
	void * exitVal = &(exitInt);
	uthread_exit(exitVal);
}

int main(int argc, char **argv){
	//multiThreadedHelloWorld();
	uthread_init(100000);
	return 0;
}
