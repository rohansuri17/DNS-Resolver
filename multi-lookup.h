#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "util.h"

typedef struct shared_index_struct
{
	void* contains;
	
} SHARED_ARRAY_INDEX;

typedef struct shared_struct
{
	SHARED_ARRAY_INDEX* array;
	int head;
	int tail;
	int max;
} SHARED_ARRAY;



struct ARRAY
{
	int size;
	FILE *arr[5];
};

struct Requester_td
{
	int thread_count;
	int serviced;
	struct ARRAY input_files;
	SHARED_ARRAY* shared;


};

struct Resolver_td
{
	FILE* output;
	SHARED_ARRAY *shared;
};

int full(SHARED_ARRAY* arr);
int empty(SHARED_ARRAY* arr);
void* rm(SHARED_ARRAY* arr);
void clear(SHARED_ARRAY* arr);
int add(SHARED_ARRAY* arr, void* new_index);
int create(SHARED_ARRAY* arr, int length);
void* requester(void* threadargs);
void *resolver(void *threadargs);