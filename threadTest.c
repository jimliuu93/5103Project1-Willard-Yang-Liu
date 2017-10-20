#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 10000
void *threadExit(void *threadId){
	pthread_exit(NULL);
}
int main(int argc, char *argv[]){
	pthread_t threads[NUM_THREADS];
	int rc;
	char * b;
	clock_t begin = clock();
	for(long i = 0; i < NUM_THREADS; i++){
		rc = pthread_create(&threads[i], NULL, threadExit, (void *)i);	
	}
	for(int j = 0; j < NUM_THREADS; j++){
		rc = pthread_join(threads[j], (void **)&b);	
	} 
	clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("time spent: %f\n", time_spent);
	return 0;
}
