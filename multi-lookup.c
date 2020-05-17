#include "multi-lookup.h"
#include <stdlib.h>
#include <pthread.h>
#include "util.h"
int finished = 0;
int dnslookup(const char* hostname, char* firstIPstr, int maxSize){

    /* Local vars */
    struct addrinfo* headresult = NULL;
    struct addrinfo* result = NULL;
    struct sockaddr_in* ipv4sock = NULL;
    struct in_addr* ipv4addr = NULL;
    char ipv4str[INET_ADDRSTRLEN];
    char ipstr[INET6_ADDRSTRLEN];
    int addrError = 0;

    /* DEBUG: Print Hostname*/
#ifdef UTIL_DEBUG
    fprintf(stderr, "%s\n", hostname);
#endif
   
    /* Lookup Hostname */
    addrError = getaddrinfo(hostname, NULL, NULL, &headresult);
    if(addrError){
	fprintf(stderr, "Error looking up Address: %s\n",
		gai_strerror(addrError));
	return UTIL_FAILURE;
    }
    /* Loop Through result Linked List */
    for(result=headresult; result != NULL; result = result->ai_next){
	/* Extract IP Address and Convert to String */
	if(result->ai_addr->sa_family == AF_INET){
	    /* IPv4 Address Handling */
	    ipv4sock = (struct sockaddr_in*)(result->ai_addr);
	    ipv4addr = &(ipv4sock->sin_addr);
	    if(!inet_ntop(result->ai_family, ipv4addr,
			  ipv4str, sizeof(ipv4str))){
		perror("Error Converting IP to String");
		return UTIL_FAILURE;
	    }
#ifdef UTIL_DEBUG
	    fprintf(stdout, "%s\n", ipv4str);
#endif
	    strncpy(ipstr, ipv4str, sizeof(ipstr));
	    ipstr[sizeof(ipstr)-1] = '\0';
	}
	else if(result->ai_addr->sa_family == AF_INET6){
	    /* IPv6 Handling */
#ifdef UTIL_DEBUG
	    fprintf(stdout, "IPv6 Address: Not Handled\n");
#endif
	    strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
	    ipstr[sizeof(ipstr)-1] = '\0';
	}
	else{
	    /* Unhandlded Protocol Handling */
#ifdef UTIL_DEBUG
	    fprintf(stdout, "Unknown Protocol: Not Handled\n");
#endif
	    strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
	    ipstr[sizeof(ipstr)-1] = '\0';
	}
	/* Save First IP Address */
	if(result==headresult){
	    strncpy(firstIPstr, ipstr, maxSize);
	    firstIPstr[maxSize-1] = '\0';
	}
    }

    /* Cleanup */
    freeaddrinfo(headresult);

    return UTIL_SUCCESS;
}
int full(SHARED_ARRAY* arr)
{
	if((arr->head==arr->tail) && (arr->array[arr->head].contains != NULL))
	{
		return 1;
	}
	else 
	{
		return 0;
	}
}
int empty(SHARED_ARRAY* arr)
{
	if((arr->head==arr->tail) && (arr->array[arr->head].contains == NULL))
	{
		return 1;
	}
	else 
	{
		return 0;
	}

}
void* rm(SHARED_ARRAY* arr)
{
	int debug = 0;
	void* remover;
	if(empty(arr))
	{
		if(debug)
		{
			printf("Debugging... shared array is empty, no item to remove\n");
		}
		return NULL;
	}
	remover = arr->array[arr->head].contains;
	arr->array[arr->head].contains = NULL;
	arr->head = ((arr->head)+1)%arr->max;
	return remover;

}
void clear(SHARED_ARRAY* arr)
{
	while(!empty(arr))
	{
		rm(arr);
	}
	free(arr->array);
}
int add(SHARED_ARRAY* arr, void* new_index)
{
	int debug =0;
	if(full(arr))
	{
		if(debug)
		{
			printf("Debugging... array is full can't add item\n");
		}
		return -1;
	}

	arr->array[arr->tail].contains =new_index;
	arr->tail = (arr->tail+1) % arr-> max;
	if(debug)
	{
		printf("Debugging... Add to shared array successful\n");
	}

	return 0;
}
int create(SHARED_ARRAY* arr, int length)
{
	int debug = 0;
	if(length>0)
	{
		arr->max = length;
	}
	else
	{
		arr->max = 100;
	}
	arr->array = malloc(sizeof(SHARED_ARRAY_INDEX) *(arr->max));
	if(!(arr->array))
	{
		printf("Error during malloc...returning\n");
		return -1;
	}
	if(debug)
	{
		printf("Debugging...malloced space successfully");
	}
	for(int i = 0; i < arr->max; i++)
	{
		arr->array[i].contains = NULL;
	}
	if(debug)
	{
		printf("Debugging... all array values set to null");
	}
	arr->head = 0;
	arr->tail = 0;

	return arr->max;
}

void* requester(void* threadargs)
{
	//int thread_count; //thread num
	struct Requester_td *Requester_td;
	int servicedFiles;
	//struct ARRAY input_files;
	SHARED_ARRAY* s;
	int debug = 1;
	FILE* fr = NULL;
	FILE* curr_file = NULL;
	pthread_mutex_t serviceM;//mutex when we are writing to serviced.txt 
	pthread_mutex_t read_shared;//when reading shared arr
	pthread_mutex_t write_shared;//when writing to shared arr

	char domainName[1025];
	FILE* input_files;
	//SHARED_ARRAY* shared;
	char* index;
	int thread_count;

	Requester_td = (struct Requester_td*) threadargs;

	pid_t requester_tid = syscall(__NR_gettid);
	//printf()

	if(debug)
	{
		printf("Debugging...requester thread id: %d \n",requester_tid);

	}

	pthread_mutex_lock(&serviceM);

	fr = fopen("serviced.txt","ab+");//read/write update append at eof

	if(!fr)
	{
		printf("Error with opening file");
		//return 0;
	}
	fprintf(fr,"Request Thread %d, serviced %d files\n",requester_tid, Requester_td->serviced);

	fclose(fr);

	pthread_mutex_unlock(&serviceM);
	for(int i = 0; i < Requester_td->input_files.size;i++)
	{

		curr_file = Requester_td->input_files.arr[i];
		s = Requester_td->shared;
		thread_count = Requester_td->thread_count;

		if(!curr_file)
		{
			printf("Can't open input file");
			//printf("%d\n",i);
			pthread_exit(NULL);
		}
		if(debug)
		{
			printf("Debugging.. File is open and being read");
		}
		while(fscanf(curr_file,"%1024s",domainName)>0)
		{
			int added = 0;
			while(!added)
			{
				pthread_mutex_lock(&read_shared);
				pthread_mutex_lock(&write_shared);
				if(!full(s))
				{
					index = malloc(1025);
					strncpy(index,domainName,1025);
					if(debug)
					{
						printf("Debugging.. Adding %s to shared array",index);
					}
					add(s,index);
				}
				added = 1;
			}
			pthread_mutex_unlock(&write_shared);
			pthread_mutex_unlock(&read_shared);
			if(!added)
			{
				usleep((rand()%100)*100000);
			}


		}
		fclose(curr_file);

	}
	if(debug)
	{
		printf("Debugging... %s is done along with thread %d",__func__,thread_count);
	}

	//FILE *perf = fopen("performance.txt","ab+");
	//fprintf(perf, "Number of requester thread = %d\n", requester_tid);
	//fclose(perf);

	return NULL;


}

void *resolver(void *threadargs)
{
	pthread_mutex_t(m);
	pthread_mutex_t(read_shared);
	pthread_mutex_t(write_shared);
	pthread_mutex_t(shared);
	pthread_mutex_t(error);

	struct Resolver_td *Resolver_td;
	FILE* output_file;
	char ip_length[INET6_ADDRSTRLEN];
	char *domainName;
	SHARED_ARRAY* s;
	Resolver_td = (struct Resolver_td*) threadargs;
	output_file = Resolver_td->output;
	s = Resolver_td -> shared;
	pid_t resolver_tid = syscall(__NR_gettid);
	printf("Resolver thread %d says Hello world in %s\n",resolver_tid,__func__);
	int is_empty = 0; 
	//int finished = 0;
	int count = 0;
	int debug = 1;
	while(!is_empty || !finished)
	{
		int resolved = 0;
		pthread_mutex_lock(&read_shared);
		pthread_mutex_lock(&m);
		count+=1;
		if(debug)
		{
			//printf("Debugging...Thread %d is trying to read and write at the same time so we have a mutex lock",resolver_tid);


		}
		if(count ==1)
		{
			pthread_mutex_lock(&write_shared);
		}
		pthread_mutex_unlock(&m);
		pthread_mutex_unlock(&read_shared);
		pthread_mutex_lock(&shared);
		is_empty = empty(s);

		if(!is_empty)
		{
			domainName = rm(s);
			if(domainName!=NULL)
			{
				if(debug)
				{
					printf("Debugging...Searching IP %s\n", domainName);
				}
				resolved = 1;

			}
		}
		pthread_mutex_unlock(&shared);

		pthread_mutex_lock(&m);
		count -=1;
		if(count == 0)
		{
			pthread_mutex_unlock(&write_shared);
		}
		pthread_mutex_unlock(&m);
		
		if(resolved)
		{
			SHARED_ARRAY IPs;
			create(&IPs,20);
			pthread_mutex_lock(&error);
			if(dnslookup(domainName,ip_length,sizeof(ip_length))==UTIL_FAILURE)
			{
				fprintf(stderr, "bogus hostname%s\n", domainName);
				strncpy(ip_length,"",sizeof(ip_length));
			}
			fprintf(output_file,"%s, %s\n", domainName,ip_length);
			if(debug)
			{
				printf("Debugging...writing to output.txt: %s, %s\n", domainName,ip_length);
			}
			pthread_mutex_unlock(&error);
			free(domainName);
			clear(&IPs);


		}





	}
	if(debug)
	{
		printf("Debugging... %s is closing and thread is sleeping\n",__func__);
	}
	//pthread_mutex_lock(read_shared);
	//pthread_mutex_lock(m);
	//FILE *output2 = fopen("performance.txt","ab+");
	//fprintf(output2,"Number for resolver thread = %d\n",resolver_tid);
	//fclose(output2);
	return NULL;

}

int main(int argc, char* argv[])
{
	int debug = 0;
	int f = 0;
	struct timeval tv;
	int start = gettimeofday(&tv,NULL);
	int request_threads = -1;
	//int finished = 0;
	remove("serviced.txt");
	remove("performance.txt");

	FILE *output_file = NULL;
	if(argc < 5)
	{
		fprintf(stderr, "There are less than 5 arguments: %d\n", argc-1);
		//fprintf(stderr, "%s\n");
		return -1;

	}
	output_file = fopen(argv[(argc-1)],"w");
	if(!output_file)
	{
		printf("Error opening output file\n");
		return -1;
	}
	SHARED_ARRAY s;
	const int size = 20;
	if(create(&s,size)==-1)
	{
		fprintf(stderr, "Unable to create shared array\n");

	}

	struct Requester_td Requester_td[10];
	pthread_t Requester_threads[10];
	int tid;
	int count;
	sscanf(argv[1],"%d",&request_threads);
	request_threads = 10;
	request_threads = (request_threads > 5) ? 5:request_threads;
	
	debug = 1;
	if(debug)
	{
		printf("Debugging...Number of request threads is %d\n", request_threads);
	}
	
	if(request_threads==-1)
	{
		printf("\nLoser\n");
	}
	
	int fpt[request_threads];
	int files = 5;

	if(files > request_threads)
	{
		int remaining  = 5;
		int actual_fpt = 5/request_threads;
		for(int i = 0; i < request_threads;i++)
		{
			fpt[i]=actual_fpt;
			remaining-=actual_fpt;

		}
		fpt[0]+=remaining;

	}
	else
	{
		for(int i = 0; i< request_threads;i++)
		{
			fpt[i] = 1;
		}
	}

	int offset = 0;

	for(count = 0; count < request_threads && count < 10; count++)
	{
		Requester_td[count].shared = &s;
		Requester_td[count].input_files.size = fpt[count];
		f = fpt[count];
		for(int i = 0; i < f; i++)
		{
			Requester_td[count].input_files.arr[i]=fopen(argv[count+offset+3],"r");
			//printf("%s\n",argv[count+offset+3]);
			if(f > 1 && i!=f-1)
			{
				offset +=1;
			}

		}
		
	}

	
	Requester_td[count].thread_count = count;
	Requester_td[count].serviced = f;
	if(debug)
	{
		printf("Request Thread %d\n",count+1);
		printf("Number of files serviced in Requester: %d\n", f);
	}

	tid = pthread_create(&(Requester_threads[count]),NULL, requester,&(Requester_td[count]));
	if(tid)
	{
		printf("Error in return code from create function %d",tid);
		return -1;
	}

	struct Resolver_td Resolver_td;
	int resolver_threads;
	int tid2;
	sscanf(argv[2],"%d",&resolver_threads);
	pthread_t Resolver_threads[resolver_threads];
	int count2;
	Resolver_td.shared = &s;
	Resolver_td.output = output_file;
	
	for(count2 = 0; count2<resolver_threads;count2++)
	{
		if(debug)
		{
			printf("Debugging...Resolver Td number %d\n",count2);
		}
		tid2 = pthread_create(&(Resolver_threads[count2]),NULL,resolver,&Resolver_td);
		if(tid2)
		{
			printf("Error in return code from create function %d\n",tid2);
			exit(-1);
		}

	}
	
	for(count = 0; (count < request_threads) && (count<10); count ++ )
	{
		pthread_join(Requester_threads[count], NULL);
	}

	
	if(debug)
	{
		printf("Debugging...Resolver threads are joined\n");
	}

	finished = 1;

	for(count = 0; count < resolver_threads; count++)
	{
		pthread_join(Resolver_threads[count],NULL);
	}

	if(debug)
	{
		printf("Resolver threads are done!!!\n");
	}
	
	fclose(output_file);
	clear(&s);

	//int end = gettimeofday();

	//int runtime = (end-start);
	//printf("Runtime is %d seconds", runtime/1000);

	FILE* output3 = fopen("performance.txt","ab+");
	//fprintf("Runtime is %d\n seconds",runtime/1000);
	fclose(output3);
	
	return 0;


}