//gcc -Wall -std=c99 -pthread blackSea-v09.c -D_POSIX_C_SOURCE=199309L

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>


clock_t startSim_t, endSim_t;
struct timespec start, finish;
int endOfSim_t;

//pthread attribute  
pthread_attr_t attr;

typedef struct Tour 
{
  	int *seats;
  	int *status_seats;

	sem_t is_full;

  	pthread_mutex_t tour_mutex;

} Tour;


Tour blackSea;

int nseats;
int npassengers;


void reserve_seat(int id)
{
	struct timespec now;

      sem_wait(&(blackSea.is_full));		 // wait if full
	clock_gettime(CLOCK_MONOTONIC, &now);
	if((now.tv_sec - start.tv_sec)>5)
		return;

	pthread_mutex_lock(&(blackSea.tour_mutex));

	// find the first empty seat and reserve it
   	for(int i=0; i<nseats; i++)
		if(blackSea.status_seats[i]==0)
		{
        		printf("passenger %d reserved seat: %d\n", id, i);
			blackSea.seats[i] = id;
			blackSea.status_seats[i] = 1;
			break;
		}

      pthread_mutex_unlock(&(blackSea.tour_mutex));

}

void buy_seat(int id, int rsrvd)
{
	struct timespec now;

	if(rsrvd==0)						// wait only if full and also
      	sem_wait(&(blackSea.is_full));		// the passenger has not made a reserve

	clock_gettime(CLOCK_MONOTONIC, &now);
	if((now.tv_sec - start.tv_sec) > endOfSim_t)		// waiting while simulation ends
		return;

	pthread_mutex_lock(&(blackSea.tour_mutex));

	// buy the reserved seat or find the first empty seat and reserve it
	if(rsrvd==0)
   		for(int i=0; i<nseats; i++)
			if(blackSea.status_seats[i]==0)
			{
        			printf("passenger %d bought seat: %d\n", id, i);
				blackSea.seats[i] = id;
				blackSea.status_seats[i] = 2;
				break;
			}
	else
	   	for(int i=0; i<nseats; i++)
			if(blackSea.seats[i]==id)		
			{
        			printf("passenger %d bought seat: %d\n", id, i);

				blackSea.status_seats[i] = 2;
				break;			
			}
      pthread_mutex_unlock(&(blackSea.tour_mutex));

}


void cancel_seat(int id)
{

	pthread_mutex_lock(&(blackSea.tour_mutex));

	// consume the last inserted item, if buffer empty something went wrong!
   	for(int i=0; i<nseats; i++)
		if(blackSea.seats[i]==id)
		{
        		printf("passenger %d cancelled its seat: %d\n", id, i);
			blackSea.seats[i] = -1;
			blackSea.status_seats[i]=0;
			break;
		}

      pthread_mutex_unlock(&(blackSea.tour_mutex));

	sem_post(&(blackSea.is_full));

}


void view_seats(int id)
{
    
	printf("passenger %d just checked the seats\n", id);
	for(int i=0; i<nseats; i++)
        if(blackSea.seats[i]);

}

void *passenger_func( void *arg ) 
{
	struct timespec now;

	int ID = *((int *) arg);
	int hasReserved = 0;
	int hasBought = 0;

	float ticket_operation;
	clock_gettime(CLOCK_MONOTONIC, &now);

	while((now.tv_sec - start.tv_sec) < endOfSim_t)
	{
		ticket_operation = (rand()/(float)(RAND_MAX));	  // generate a random ticket operation

		usleep(rand()%400000);	// sleep some random time (to avoid having a super big number of actions)

		if(ticket_operation < 0.4)
		{
			if(hasReserved==0 && hasBought==0)
			{
				reserve_seat(ID);
				hasReserved = 1;
			}
		} 
		else if(0.4<=ticket_operation  &&  ticket_operation< 0.6)
		{
			if(hasReserved==1 || hasBought==1)
			{
				cancel_seat(ID);
				hasReserved = 0;
				hasBought = 0;
			}
		} 
		else if(0.6 <= ticket_operation  &&  ticket_operation<0.8)
		{
			view_seats(ID);
		}
		else
		{
			if(hasBought==1)
				continue;
			buy_seat(ID, hasReserved);
			hasBought = 1;
			hasReserved = 1;
		}
		clock_gettime(CLOCK_MONOTONIC, &now);
	}
	if(hasReserved==1 || hasBought==1)
		sem_post(&(blackSea.is_full));

	pthread_exit(NULL);
}

void *log_func(void *arg)
{
	sleep(1);
	printf("LOGGING...");
	// logStats();
	sleep(1);
	printf("LOGGING...");
	// logStats();
	sleep(1);
	printf("LOGGING...");
	// logStats();
	sleep(1);
	printf("LOGGING...");
	// logStats();
	sleep(1);
	printf("LOGGING...");
	// logStats();

	pthread_exit(NULL);
}

int main() 
{

	clock_gettime(CLOCK_MONOTONIC, &start);
	
	nseats = 10;
	npassengers = 20;
	endOfSim_t = 5;

	blackSea.seats = (int*)malloc(nseats*sizeof(int));
	blackSea.status_seats = (int*)malloc(nseats*sizeof(int));
	for(int i=0; i<nseats; i++)
	{
		blackSea.seats[i] = -1;
		blackSea.status_seats[i]=0;
	}		

	// Setup
    	pthread_mutex_init(&(blackSea.tour_mutex), NULL);

    	sem_init(&(blackSea.is_full), 0, nseats);

    	pthread_attr_init(&attr);

	pthread_t *threadPssngrs = (pthread_t *)malloc(npassengers*sizeof(pthread_t));
	pthread_t logThread;
	int *pssngr_id = (int *)malloc(npassengers*sizeof(int));

	// pthread_create(&logThread,&attr,log_func,NULL);	// a thread that does logging every one second

    	// Threads of Passengers creation
    	for(int i = 0; i < npassengers; i++) 
	{
		pssngr_id[i] = i;
        	pthread_create(&threadPssngrs[i],&attr,passenger_func,(void *)&pssngr_id[i]);

    	}


    	for(int i = 0; i < npassengers; i++) 
	{
        	pthread_join(threadPssngrs[i],NULL);
    	}
	//pthread_join(logThread,NULL);

	pthread_mutex_destroy(&(blackSea.tour_mutex));

	free(threadPssngrs);
	free(pssngr_id);
    	printf("Seats reservation finished\n");
	clock_gettime(CLOCK_MONOTONIC, &finish);
	double elapsed;
	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
	printf("CLOCK_CPU: %lf\n",elapsed);
    	exit(0);
}



