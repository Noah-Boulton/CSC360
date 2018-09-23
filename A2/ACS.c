/* Noah Boulton - V00803598
   CSC 360 - P2 Fall 2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include "queue.h"


void *clerk_entry(void * clerkNum);
void *customer_entry(void * cus_info);
double getCurrentSimulationTime();
int pickShortest();
void lock(int n);
void unlock(int n);


struct customer_info{ //use this struct to record the customer information read from customers.txt
    int user_id;
	int service_time;
	int arrival_time;
	double start;
	double end;
	int shortest;
};

struct clerk_info{
	int clerk_id;
};
/* global variables */

int NClerks = 2;
const static int NQUEUE = 4;

struct timeval init_time; // use this variable to record the simulation start time; No need to use mutex_lock when reading this variable since the value would not be changed by thread once the initial time was set.
double overall_waiting_time; //A global variable to add up the overall waiting time for all customers, every customer add their own waiting time to this variable, mutex_lock is necessary.
int queue_length[4];// variable stores the real-time queue length information; mutex_lock needed

Queue* queues[4];

//pthread_mutex_t queue_mutex[4];
pthread_mutex_t queue_0_mutex;
pthread_mutex_t queue_1_mutex;
pthread_mutex_t queue_2_mutex;
pthread_mutex_t queue_3_mutex;

pthread_mutex_t time_mutex;
pthread_mutex_t clerk_0_mutex;
pthread_mutex_t clerk_1_mutex;

pthread_cond_t queue_0_cv;
pthread_cond_t queue_1_cv;
pthread_cond_t queue_2_cv;
pthread_cond_t queue_3_cv;

pthread_cond_t clerk_0_cv;
pthread_cond_t clerk_1_cv;

//Keep track of which clerk is service currently
int sending;

//Number of customers 
int NCustomers;

// function entry for customer threads
void *customer_entry(void * cus_info){
	
	struct customer_info * p_myInfo = (struct customer_info *) cus_info;

	//usleep unitl the customer arives 
	usleep(p_myInfo->arrival_time * 100000);

	fprintf(stdout, "A customer arrives: customer ID %2d. \n", p_myInfo->user_id);

	//lock the queues to check their length
	int n;
	for(n = 0; n < 4; n++){
		lock(n);
	}

	//pick shortest queue
	p_myInfo->shortest = pickShortest();

	//Unlock all but the chosen queue
	for(n = 0; n < 4; n++){
		if(p_myInfo->shortest != n){
			unlock(n);
		}
	}

	//update queue length
	enqueue(queues[p_myInfo->shortest], p_myInfo->user_id);

	fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", p_myInfo->shortest, queues[p_myInfo->shortest]->size);

	//Set the start time for customer 
	p_myInfo->start = getCurrentSimulationTime();

	//wait for the clerk to signal and you are at the head of the queue
	if(p_myInfo->shortest == 0){
		do{
			if(pthread_cond_wait(&queue_0_cv, &queue_0_mutex)){
				printf("Error on condition wait.\n");
				exit(0);
			}
		} while(p_myInfo->user_id != front(queues[p_myInfo->shortest]));
	} else if(p_myInfo->shortest == 1){
		do{
			if(pthread_cond_wait(&queue_1_cv, &queue_1_mutex)){
				printf("Error on condition wait.\n");
				exit(0);
			}
		} while(p_myInfo->user_id != front(queues[p_myInfo->shortest]));
	} else if(p_myInfo->shortest == 2){
		do{
			if(pthread_cond_wait(&queue_2_cv, &queue_2_mutex)){
				printf("Error on condition wait.\n");
				exit(0);
			}
		} while(p_myInfo->user_id != front(queues[p_myInfo->shortest]));
	} else if(p_myInfo->shortest == 3){
		do{
			if(pthread_cond_wait(&queue_3_cv, &queue_3_mutex)){
				printf("Error on condition wait.\n");
				exit(0);
			}
		} while(p_myInfo->user_id != front(queues[p_myInfo->shortest]));
	}

	//Now pthread_cond_wait returned, customer was awoken by one of the clerks
	//remove yourself from the queue
	int dequeued = dequeue(queues[p_myInfo->shortest]);
	if(dequeued != p_myInfo->user_id){
		printf("Error in dequeue\n");
		exit(0);
	}

	//Unlock the queue you are in
	unlock(p_myInfo->shortest);

	//figure out who is serving yous
	int awoken = sending;
	sending = -1;
	if(awoken == -1){
		printf("Error\n");
		exit(0);
	}

	//Record the end waiting time
	p_myInfo->end = getCurrentSimulationTime();

	//Add the customers waiting time to the overall time
	if(pthread_mutex_lock(&time_mutex)){
		printf("Error locking time mutex.\n");
		exit(0);
	}

	overall_waiting_time += p_myInfo->end - p_myInfo->start;
	
	if(pthread_mutex_unlock(&time_mutex)){
		printf("Error locking time mutex.\n");
		exit(0);
	}
	
	fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->end, p_myInfo->user_id, awoken);
	
	//usleep for the service time of the customer 
	usleep(p_myInfo->service_time * 100000);
	
	//Note the end of service time
	double end_serve = getCurrentSimulationTime();

	fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_serve, p_myInfo->user_id, awoken);\

	//Signal the clerk that the customer is done 
	if(awoken == 1){ //clerk1 signaled you
		if(pthread_mutex_lock(&clerk_0_mutex)){
			printf("Error locking clerk 0 mutex.\n");
			exit(0);
		}
		if(pthread_cond_signal(&clerk_0_cv)){
			printf("Error on condition signal.\n");
			exit(0);
		}
		if(pthread_mutex_unlock(&clerk_0_mutex)){
			printf("Error unlocking clerk 0 mutex.\n");
			exit(0);
		}
	} else if(awoken == 2){//clerk2 signaled you 
		if(pthread_mutex_lock(&clerk_1_mutex)){
			printf("Error locking clerk 1 mutex.\n");
			exit(0);
		}
		if(pthread_cond_signal(&clerk_1_cv)){
			printf("Error on condition signal.\n");
			exit(0);
		}
		if(pthread_mutex_unlock(&clerk_1_mutex)){
			printf("Error unlocking clerk 1 mutex.\n");
			exit(0);
		}
	} else {
		printf("Error in clerk to customer signal\n");
		exit(0);
	}

	pthread_exit(NULL);
	
	return NULL;
}

// function entry for clerk threads
void *clerk_entry(void * clerkNum){
	struct clerk_info * p_myInfo = (struct clerk_info *) clerkNum;
	int clerkId = p_myInfo->clerk_id;

	while(1){
		// clerk is idle now

		/* Check if there are customers waiting in queues, if so, pick the queue with the longest customers. */

		//lock the queues before selecting the longest one
		int n;
		for(n = 0; n < 4; n++){
			lock(n);
		}

		int longest = pickLongest();

		//unlock all but the longest queue
		for(n = 0; n < 4; n++){
			if(longest != n){
				unlock(n);
			}
		}

		//If no customers in queue then usleep(10); and check again
		if(longest == -1){
			//make sure to unlock the remaining locked queue
			unlock(longest);
			usleep(10);
			continue;
		}

		//Wait until the other clerk is finished signalling thier customer

		//Could use A and B here to see if they are about to signal the same queue in which case get them to pick again?
		while(sending != -1){
			usleep(10);
		}
		sending = clerkId;

		//Signal the customer in the longest queue
		if(longest == 0){
			if(pthread_cond_signal(&queue_0_cv)){
				printf("Error on condition signal.\n");
				exit(0);
			}
		}
		if(longest == 1){
			if(pthread_cond_signal(&queue_1_cv)){
				printf("Error on condition signal.\n");
				exit(0);
			}
		}
		if(longest == 2){
			if(pthread_cond_signal(&queue_2_cv)){
				printf("Error on condition signal.\n");
				exit(0);
			}
		}
		if(longest == 3){
			if(pthread_cond_signal(&queue_3_cv)){
				printf("Error on condition signal.\n");
				exit(0);
			}
		}

		//Unlock the longest queue
		unlock(longest);

		//Wait for the customer to finish service
		if(clerkId == 1){
			if(pthread_mutex_lock(&clerk_0_mutex)){
				printf("Error locking clerk 1 mutex.\n");
				exit(0);
			}
			if(pthread_cond_wait(&clerk_0_cv, &clerk_0_mutex)){
				printf("Error on condition signal.\n");
				exit(0);
			}
			if(pthread_mutex_unlock(&clerk_0_mutex)){
				printf("Error unlocking clerk 1 mutex.\n");
				exit(0);
			}
		} else if(clerkId == 2){
			if(pthread_mutex_lock(&clerk_1_mutex)){
				printf("Error locking clerk 1 mutex.\n");
				exit(0);
			}
			if(pthread_cond_wait(&clerk_1_cv, &clerk_1_mutex)){
				printf("Error on condition signal.\n");
				exit(0);
			}
			if(pthread_mutex_unlock(&clerk_1_mutex)){
				printf("Error unlocking clerk 1 mutex.\n");
				exit(0);
			}
		}
	}
	return NULL;
}

//Function used to lock a spefific queue 
void lock(int n){
	if(n == 0){
			if(pthread_mutex_lock(&queue_0_mutex)){
				printf("Error locking queue mutex 0.\n");
				exit(0);
			}
		}
		if(n == 1){
			if(pthread_mutex_lock(&queue_1_mutex)){
				printf("Error locking queue mutex 1.\n");
				exit(0);
			}
		}
		if(n == 2){
			if(pthread_mutex_lock(&queue_2_mutex)){
				printf("Error locking queue mutex 2.\n");
				exit(0);
			}
		}
		if(n == 3){
			if(pthread_mutex_lock(&queue_3_mutex)){
				printf("Error locking queue mutex 3.\n");
				exit(0);
			}
		}
}

//Function used to unlock a spefific queue 
void unlock(int n){
	if(n == 0){
			if(pthread_mutex_unlock(&queue_0_mutex)){
				printf("Error unlocking queue mutex 0.\n");
				exit(0);
			}
		}
		if(n == 1){
			if(pthread_mutex_unlock(&queue_1_mutex)){
				printf("Error unlocking queue mutex 1.\n");
				exit(0);
			}
		}
		if(n == 2){
			if(pthread_mutex_unlock(&queue_2_mutex)){
				printf("Error unlocking queue mutex 2.\n");
				exit(0);
			}
		}
		if(n == 3){
			if(pthread_mutex_unlock(&queue_3_mutex)){
				printf("Error unlocking queue mutex 3.\n");
				exit(0);
			}
		}
}

//Function used to select the shortest queue
int pickShortest(int id){
	int i;
	int min = NCustomers + 1;
	int shortest = -1;
	int tied = 0;
	for(i = 0; i < 4; i++){
		if(queues[i]->size < min){
			min = queues[i]->size;
			shortest = i;
		}
	}
	for(i = 0; i < 4; i++){
		if(queues[i]->size == min){
			tied++;
		}
	}

	if(tied == 1){
		return shortest;
	}

	int arr[tied];
	int j = 0;
	for(i = 0; i < 4; i++){
		if(queues[i]->size == min){
			arr[j] = i;
			j++;
		}
	}
	return pickRand(tied, arr);
}

//Function used to select the longest queue
int pickLongest(){
	int i;
	int max = 0;
	int longest = -1;
	int tied = 0;
	for(i = 0; i < 4; i++){
		if(queues[i]->size > max){
			max = queues[i]->size;
			longest = i;
		}
	}

	if(longest == -1){
		return longest;
	}

	for(i = 0; i < 4; i++){
		if(queues[i]->size == max){
			tied++;
		}
	}

	if(tied == 1){
		return longest;
	}

	int arr[tied];
	int j = 0;
	for(i = 0; i < 4; i++){
		if(queues[i]->size == max){
			arr[j] = i;
			j++;
		}
	}
	return pickRand(tied, arr);
}

//Used to pick a random queue
int pickRand(int num, int arr[]){
	int r = rand() % num;
	return arr[r];
}

//Used to calculate the current system time
double getCurrentSimulationTime(){
	
	struct timeval cur_time;
	double cur_secs, init_secs;
	
	init_secs = (init_time.tv_sec + (double) init_time.tv_usec / 1000000);
	
	gettimeofday(&cur_time, NULL);
	cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);
	
	return cur_secs - init_secs;
}

int main(int argc, char const *argv[]){

	//Read from the input file
	FILE *f = fopen(argv[1], "r");
	if(f == NULL){
                printf("Error reading from file.\n");
                return 0;
    }
    char buff[255];
    fgets(buff, 255, (FILE*)f);
    NCustomers = atoi(buff);
    struct customer_info customers[NCustomers];
    int i = 0;
    char token[4];

    //Build customer structs from the input file, if a negative value is entered for arrival or service time then exit the program
    while(fgets(buff, 255, (FILE*)f)) {
    	strcpy(token, strtok(buff, ":,"));
    	int u_id = atoi(token);
    	if(u_id < 0){
    		printf("Error: Customer id of %d not valid, line skipped.\n", u_id);
    		NCustomers--;
    		continue;
    	}
    	
    	strcpy(token, strtok(NULL, ":,"));
    	int a_time = atoi(token);
    	if(a_time < 0){
    		printf("Error: Arrival time of %d not valid, line skipped.\n", a_time);
    		NCustomers--;
    		continue;
    	}
    	
    	strcpy(token, strtok(NULL, ":,"));
    	int s_time = atoi(token);
    	if(s_time < 0){
    		printf("Error: Service time of %d not valid, line skipped.\n", s_time);
    		NCustomers--;
    		continue;
    	}
    	customers[i].user_id = u_id;
    	customers[i].arrival_time = a_time;
    	customers[i].service_time = s_time;
    	customers[i].start = 0;
    	customers[i].end = 0;
    	i++;
    }

    //Initialize sending
    sending = -1;

	//Initialize random number generator 
	srand(time(NULL)); 
	for(i = 0; i < 4; i++){
		queues[i] = createQueue(NCustomers+1);
	}

	//init mutexes and convars
	if(pthread_mutex_init(&clerk_0_mutex, NULL)){
		printf("Error initializing clerk mutex.\n");
		exit(0);
	}

	if(pthread_mutex_init(&clerk_1_mutex, NULL)){
		printf("Error initializing clerk mutex.\n");
		exit(0);
	}

	if(pthread_mutex_init(&queue_0_mutex, NULL)){
			printf("Error initializing queue mutex.\n");
			exit(0);
	}
	if(pthread_cond_init(&queue_0_cv, NULL)){
  		printf("Error at queue_cv init\n");
  		exit(0);
  	}

  	if(pthread_mutex_init(&queue_1_mutex, NULL)){
			printf("Error initializing queue mutex.\n");
			exit(0);
	}
	if(pthread_cond_init(&queue_1_cv, NULL)){
  		printf("Error at queue_cv init\n");
  		exit(0);
  	}

  	if(pthread_mutex_init(&queue_2_mutex, NULL)){
			printf("Error initializing queue mutex.\n");
			exit(0);
	}
	if(pthread_cond_init(&queue_2_cv, NULL)){
  		printf("Error at queue_cv init\n");
  		exit(0);
  	}

  	if(pthread_mutex_init(&queue_3_mutex, NULL)){
			printf("Error initializing queue mutex.\n");
			exit(0);
	}
	if(pthread_cond_init(&queue_3_cv, NULL)){
  		printf("Error at queue_cv init\n");
  		exit(0);
  	}

	if(pthread_mutex_init(&time_mutex, NULL)){
		printf("Error initializing time mutex.\n");
		exit(0);
	}

  	if(pthread_cond_init(&clerk_0_cv, NULL)){
  		printf("Error at clerk_0_cv init\n");
  		exit(0);
  	}

  	if(pthread_cond_init(&clerk_1_cv, NULL)){
  		printf("Error at clerk_1_cv init\n");
  		exit(0);
  	}
	
	pthread_t clerkId[2];
	struct clerk_info clerks[NClerks];
	//create clerk thread
	//make sure to catch return values create returns 0 on success 
	for(i = 1; i <= NClerks; i++){ // number of clerks
		clerks[i].clerk_id = i;
		if(pthread_create(&clerkId[i], NULL, clerk_entry, (void *)&clerks[i])){
			printf("Error creating clerk thread.\n");
			exit(0);
		} // clerk_info: passing the clerk information (e.g., clerk ID) to clerk thread
	}
	
	pthread_t customerId[NCustomers];

	//Initialize the simulation start time
	gettimeofday(&init_time, NULL);

	//create customer thread
	//make sure to catch return values 
	for(i = 0; i < NCustomers; i++){ // number of customers
		if(pthread_create(&customerId[i], NULL, customer_entry, (void *)&customers[i])){
			printf("Error creating customer thread.\n");
			exit(0);
		} //custom_info: passing the customer information (e.g., customer ID, arrival time, service time, etc.) to customer thread
	}
	//Wait for all customer threads to terminate
	int k;
	for( k = 0; k < NCustomers; k++){
		if(pthread_join(customerId[k], NULL)){
			printf("Error joining thread.\n");
			exit(0);
		}
	}

	printf("Overall waiting time %.2f\nAverage waiting time %.2f\n", overall_waiting_time, overall_waiting_time/NCustomers);

	return 0;
}




