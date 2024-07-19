#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


pthread_mutex_t lock_count = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_chosen_num = PTHREAD_MUTEX_INITIALIZER;


typedef struct _rwlock_t
{
    sem_t lock;
    sem_t writelock;
    int readers;

}rwlock_t;

void parseargs(char *argv[], int argc, int *lval, int *uval, int *nval, int *tval);

int  isprime(int n);

int  chooseNumFromRange();

void *funcThreads(void *arg);

void rwlock_init(rwlock_t *rw);

void rwlock_acquire_readlock(rwlock_t *rw);

void rwlock_release_readlock(rwlock_t *rw);

void rwlock_acquire_writelock(rwlock_t *rw);

void rwlock_release_writelock(rwlock_t *rw);


char 	*flagarr = NULL;
int 	lval = 1;
int 	uval = 100;
int 	nval = 10;
int 	tval = 4;
int 	num;
int 	count = 0;
int 	currentNum;

rwlock_t lock;

int main(int argc, char **argv)
{

    rwlock_init(&lock);
    parseargs(argv, argc, &lval, &uval, &nval, &tval);
    
    if (uval < lval)
    {
        fprintf(stderr, "Upper bound should not be smaller then lower bound\n");
        exit(1);
    }
    
    if (nval > uval-lval)
  	{
    		fprintf(stderr, "number of primes can't be than the numbers of values\n");
    		exit(1);
  	}
    
    if (lval < 2)
    {
        lval = 2;
        uval = (uval > 1) ? uval : 1;
    }

	currentNum = lval;
      

    //Allocate flags 
    flagarr = (char *)malloc(sizeof(char) * (uval - lval + 1));
    if (!flagarr)
        exit(1);


    pthread_t treads[tval];
    int i = 0;
    int error_create;
    int error_join;


    while (i < tval)
    {
        error_create = pthread_create(&(treads[i]), NULL, &funcThreads, NULL);
        if (error_create != 0)
            printf("\nFailed creating thread %d!", i);
        i++;
    }
    
    for (int i = 0; i < tval; i++)
    {
        pthread_join(treads[i], NULL);
    }
    
    // Print results
  	printf("Found %d primes%c\n", count, count ? ':' : '.');

  	for (num = lval; num <= uval; num++)
  	{
  			if (flagarr[num - lval] && nval != 0)
   			{
     			count--;
      			printf("%d", num);
      			nval--;
      		
      			if(nval == 0 || count == 0)
      				printf("%c", '\n');
      			else
      				printf("%c", ',');
      	}
  	}
}
    

// NOTE : use 'man 3 getopt' to learn about getopt(), opterr, optarg and optopt
void parseargs(char *argv[], int argc, int *lval, int *uval, int *nval, int *tval)
{
    int ch;

    opterr = 0;
    while ((ch = getopt(argc, argv, "l:u:n:t:")) != -1)
        switch (ch)
        {
        case 'l': // Lower bound flag
            *lval = atoi(optarg);
            break;
        case 'u': // Upper bound flag
            *uval = atoi(optarg);
            break;
        case 'n': 
            *nval = atoi(optarg);
            break;
        case 't': 
            *tval = atoi(optarg);
            break;
        case '?':
            if ((optopt == 'l') || (optopt == 'u') || (optopt == 'n') || (optopt == 't'))
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            exit(1);
        default:
            exit(1);
        }
}


int isprime(int n)
{                    
    static int *primes = NULL; // NOTE: static !
    static int size = 0;       // NOTE: static !
    static int maxprime;       // NOTE: static !
    int root;
    int i;
    
    rwlock_acquire_writelock(&lock);
    if (primes == NULL)
    {
        primes = (int *)malloc(2 * sizeof(int));
        if (!primes)
        {
        	rwlock_release_writelock(&lock);
        	exit(1);
        }

        size = 2;
        primes[0] = 2;
        primes[1] = 3;
        maxprime = 3;
    }
    rwlock_release_writelock(&lock);
    

    root = (int)(sqrt(n));


    // Update primes array, if needed
    while (root > maxprime)
        for (i = maxprime + 2;; i += 2)
        {
        
        	if (isprime(i))
                {
                	
               		rwlock_acquire_writelock(&lock);
               		size++;
                	primes = (int *)realloc(primes, size * sizeof(int));
               		if (!primes)
               		{
						rwlock_release_writelock(&lock);
						exit(1);
					}
               		primes[size - 1] = i;
               		maxprime = i;
               		rwlock_release_writelock(&lock);  
                	break;
                }
        }
        
     
    if (n <= 0)
        return -1;
    if (n == 1)
        return 0;
        
    rwlock_acquire_readlock(&lock);

    // Check prime
    for (i = 0; ((i < size) && (root >= primes[i])); i++)
        if ((n % primes[i]) == 0)
        {
            rwlock_release_readlock(&lock);
            return 0;
        }
    rwlock_release_readlock(&lock);

    return 1;
}

int chooseNumFromRange()
{
    pthread_mutex_lock(&lock_chosen_num);
	if(currentNum <= uval)
	{
		int chosenNum = currentNum;
		currentNum++;
		pthread_mutex_unlock(&lock_chosen_num);
		return chosenNum;
	}
	pthread_mutex_unlock(&lock_chosen_num);
	return -1;
}

void *funcThreads(void *arg)
{
	int num = chooseNumFromRange();
	while(num != -1)
	{
        if (isprime(num))
        {
            pthread_mutex_lock(&lock_count);
            count++;
            pthread_mutex_unlock(&lock_count);
            flagarr[num - lval] = 1;
        }
        else
        {
            flagarr[num - lval] = 0;
        }
        num = chooseNumFromRange();
    }
    

    return NULL;
}

void rwlock_init(rwlock_t *rw)
{
    rw->readers = 0;
    sem_init(&rw->lock, 0, 1);
    sem_init(&rw->writelock, 0, 1);
}

void rwlock_acquire_readlock(rwlock_t *rw)
{
    sem_wait(&rw->lock);
    rw->readers++;
    if (rw->readers == 1)
        sem_wait(&rw->writelock);
    sem_post(&rw->lock);
}

void rwlock_release_readlock(rwlock_t *rw)
{
    sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)
        sem_post(&rw->writelock);
    sem_post(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw)
{
    sem_wait(&rw->writelock);
}

void rwlock_release_writelock(rwlock_t *rw)
{
    sem_post(&rw->writelock);
}

