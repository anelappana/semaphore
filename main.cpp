//
// Example from: http://www.amparo.net/ce155/sem-ex.c
//
// Adapted using some code from Downey's book on semaphores
//
// Compilation:
//
//       g++ main.cpp -lpthread -o main -lm
// or 
//      make
//

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <iostream>
using namespace std;

/*
 This wrapper class for semaphore.h functions is from:
 http://stackoverflow.com/questions/2899604/using-sem-t-in-a-qt-project
 */
class Semaphore {
public:
    // Constructor
    Semaphore(int initialValue)
    {
        sem_init(&mSemaphore, 0, initialValue);
    }
    // Destructor
    ~Semaphore()
    {
        sem_destroy(&mSemaphore); /* destroy semaphore */
    }
    
    // wait
    void wait()
    {
        sem_wait(&mSemaphore);
    }
    // signal
    void signal()
    {
        sem_post(&mSemaphore);
    }
    
    
private:
    sem_t mSemaphore;
};

Semaphore Mutex(1);

class LightSwitch {
public:
    LightSwitch() : counter(0) {} 
    int counter = 0;
    void lock(Semaphore &semaphore) {
        Mutex.wait();
        counter++;
        if (counter == 1) {
            semaphore.wait();
        }
        Mutex.signal();
    }
    void unlock(Semaphore &semaphore) {
        Mutex.wait();
        counter--;
        if (counter == 0) {
            semaphore.signal();
        }
        Mutex.signal();
    }
};

/* global vars */
const int bufferSize = 5;
const int numConsumers = 5; 
const int numProducers = 5; 

/* semaphores are declared global so they can be accessed
 in main() and in thread routine. */

Semaphore Spaces(bufferSize);
Semaphore Items(0);

LightSwitch ReadSwitch;

Semaphore roomEmpty(1);
Semaphore turnstile(1);


LightSwitch writeSwitch;
Semaphore noReaders(1);
Semaphore noWriters(1);
Semaphore forks[5] = {Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1)};
Semaphore footman(4);


/*
    Producer function 
*/
void *Producer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
        Spaces.wait();
        Mutex.wait();
            printf("Producer %d adding item to buffer \n", x);
            fflush(stdout);
        Mutex.signal();
        Items.signal();
    }

}

/*
    Consumer function 
*/
void *Consumer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;
    
    while( 1 )
    {
        Items.wait();
        Mutex.wait();
            printf("Consumer %d removing item from buffer \n", x);
            fflush(stdout);
        Mutex.signal();
        Spaces.signal();
        sleep(5);   // Slow the thread down a bit so we can see what is going on
    }

}

void Producerconsumer() 
{
    pthread_t producerThread[ numProducers ];
    pthread_t consumerThread[ numConsumers ];

    // Create the producers 
    for( long p = 0; p < numProducers; p++ )
    {
        int rc = pthread_create ( &producerThread[ p ], NULL, 
                                  Producer, (void *) (p+1) );
        if (rc) {
            printf("ERROR creating producer thread # %d; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }

    // Create the consumers 
    for( long c = 0; c < numConsumers; c++ )
    {
        int rc = pthread_create ( &consumerThread[ c ], NULL, 
                                  Consumer, (void *) (c+1) );
        if (rc) {
            printf("ERROR creating consumer thread # %d; \
                    return code from pthread_create() is %d\n", c, rc);
            exit(-1);
        }
    }

    printf("Main: program completed. Exiting.\n");


    // To allow other threads to continue execution, the main thread 
    // should terminate by calling pthread_exit() rather than exit(3). 
    pthread_exit(NULL); 


}

// No Starvation Problem
void *nowriter(void *threadID)
{
    int x = (long)threadID;
    while (1) {
        sleep(2);
    turnstile.wait();
    roomEmpty.wait();
    printf("writer %d Writing \n", x);
    turnstile.signal();
    roomEmpty.signal();
    }
}

void *noreader(void *threadID)
{
    int x = (long)threadID;
    while (1) {
        sleep(2);
turnstile.wait();
turnstile.signal();

ReadSwitch.lock(roomEmpty);
printf("reader %d Reading \n", x);
ReadSwitch.unlock(roomEmpty);
    }
}

void nostarve()
{
    pthread_t readerThread[ numProducers ];
    pthread_t writerThread[ numConsumers ];

    // Create the producers 
    for( long p = 0; p < numProducers; p++ )
    {
        int rc = pthread_create ( &readerThread[ p ], NULL, 
                                  nowriter, (void *) (p+1) );
        if (rc) {
            printf("ERROR creating producer thread # %d; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }

    // Create the consumers 
    for( long c = 0; c < numConsumers; c++ )
    {
        int rc = pthread_create ( &writerThread[ c ], NULL, 
                                  noreader, (void *) (c+1) );
        if (rc) {
            printf("ERROR creating consumer thread # %d; \
                    return code from pthread_create() is %d\n", c, rc);
            exit(-1);
        }
    }

    printf("Main: program completed. Exiting.\n");


    // To allow other threads to continue execution, the main thread 
    // should terminate by calling pthread_exit() rather than exit(3). 
    pthread_exit(NULL);

}

//
// Write Priority Problem
//
void *wpwriter(void *threadID)
{

    int x = (long)threadID;
    while (1) {
        sleep(2);
    writeSwitch.lock(noReaders);
    noWriters.wait();
    printf("writer %d Writing \n", x);
    noWriters.signal();
    writeSwitch.unlock(noReaders);
    }
}

void *wpreader(void *threadID)
{

    int x = (long)threadID;
    while (1) {
        sleep(2);
    noReaders.wait();
    ReadSwitch.lock(noWriters);
    noReaders.signal();
    printf("reader %d Reading \n", x);
    ReadSwitch.unlock(noWriters);
    }
}

void writepriority()
{
    pthread_t readerThread[ numProducers ];
    pthread_t writerThread[ numConsumers ];

    // Create the producers 
    for( long p = 0; p < numProducers; p++ )
    {
        int rc = pthread_create ( &writerThread[ p ], NULL, 
                                  wpwriter, (void *) (p+1) );
        if (rc) {
            printf("ERROR creating producer thread # %d; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }

    // Create the consumers 
    for( long c = 0; c < numConsumers; c++ )
    {
        int rc = pthread_create ( &readerThread[ c ], NULL, 
                                  wpreader, (void *) (c+1) );
        if (rc) {
            printf("ERROR creating consumer thread # %d; \
                    return code from pthread_create() is %d\n", c, rc);
            exit(-1);
        }
    }

    printf("Main: program completed. Exiting.\n");


    // To allow other threads to continue execution, the main thread 
    // should terminate by calling pthread_exit() rather than exit(3). 
    pthread_exit(NULL);

}

//
// Dining Philosophers Problem 1
//
int left(int i)
{
    return i;
}

int right(int i)
{
    return (i + 1) % 5;
}

void get_forks(int philosopher)
{
    footman.wait();
    forks[left(philosopher)].wait();
    forks[right(philosopher)].wait();  
}

void put_forks(int philosopher)
{
    forks[left(philosopher)].signal();
    forks[right(philosopher)].signal();  
    footman.signal();
}



void *philosopher1(void *threadID)
{
    int x = (long)threadID;
    while (1) {
        sleep(3);
        printf("Philosopher %d is thinking\n", x);
        get_forks(x);
        printf("Philosopher %d is eating\n", x);
        put_forks(x);
    }
}

void dineingphilosophers1()
{
    pthread_t philosThread[ numProducers ];

    for( long p = 0; p < numProducers; p++ )
    {
        int rc = pthread_create ( &philosThread[ p ], NULL, 
                                  philosopher1, (void *) (p+1) );
        if (rc) {
            printf("ERROR creating producer thread # %d; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }
    pthread_exit(NULL);
}

//
// Dining Philosophers Problem 2
//

void get_forks2(int philosopher)
{
    if (philosopher % 2 == 0) {
        forks[right(philosopher)].wait();  
        forks[left(philosopher)].wait();
    } else {
        forks[left(philosopher)].wait();
        forks[right(philosopher)].wait();  
    }
}

void put_forks2(int philosopher)
{
    if (philosopher % 2 == 0) {
        forks[right(philosopher)].signal();
        forks[left(philosopher)].signal();
    } else {
        forks[left(philosopher)].signal(); 
        forks[right(philosopher)].signal();
    }
}

void *philosopher2(void *threadID)
{
    int x = (long)threadID;
    while (1) {
        sleep(3);
        printf("Philosopher %d is thinking\n", x);
        get_forks2(x);
        printf("Philosopher %d is eating\n", x);
        put_forks2(x);
    }
}

void dineingphilosophers2()
{
    pthread_t philosThread[ numProducers ];

    for( long p = 0; p < numProducers; p++ )
    {
        int rc = pthread_create ( &philosThread[ p ], NULL, 
                                  philosopher1, (void *) (p+1) );
        if (rc) {
            printf("ERROR creating producer thread # %d; \
                    return code from pthread_create() is %d\n", p, rc);
            exit(-1);
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv )
{

    int choice = atoi (argv[1]);
 switch (choice) {
    case 1:
        printf("Producer Consumer Problem\n");
        Producerconsumer();
        break;
    case 2:
        printf("No Starvation Problem\n");
        nostarve();
        break;
    case 3:
        printf("Write Priority Problem\n");
        writepriority();
        break;
    case 4:
        printf("Dining Philosophers Problem 1\n");
        dineingphilosophers1();
        break;
    case 5:
        printf("Dining Philosophers Problem 2\n");
        dineingphilosophers2();
        break;
    default:
        cout << "Usage: " << argv[0] << endl;
        exit(EXIT_FAILURE);
 }


} /* main() */