#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#define NUM_THREADS 2

// Global variables
pthread_t threads[NUM_THREADS];
pthread_mutex_t lock;
int inUse = 0;


struct thread_data{
   int  thread_id;
   int  accompanied;
};

void *useMicrowave(void *threadID) {
    long tID = (long)threadID;
    std::cout << "Going to heat food! Thread ID, " << tID << std::endl;

    pthread_mutex_lock(&lock);

    if(inUse == 1) {
        std::cout << "Waiting! Thread ID, " << tID << std::endl;
    } else {
        inUse = 1;
        std::cout << "Heating up! Thread ID, " << tID << std::endl;
        std::this_thread::sleep_for (std::chrono::seconds(5));
        inUse = 0;
        std::cout << "Done heating! Thread ID, " << tID << std::endl;
    }
    
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

// Main
int main(int argc, char** argv) {
    // Need to implement error treatment
    int useTimes = std::stoi(argv[1]);

    // Initialize mutex
    if (pthread_mutex_init(&lock, NULL) != 0) {
        std::cout << "Mutex init failed" << std::endl;
        return 1;
    }

    for(int i=0; i < NUM_THREADS; i++) {
        // std::cout << "Creating thread, " << i << std::endl;
        int threadError = pthread_create(&threads[i], NULL, useMicrowave, (void*) i);
      
        if(threadError) {
            std::cout << "Unable to create thread, " << threadError << std::endl;
            exit(-1);
        }
    }

    for(int i=0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    

    pthread_mutex_destroy(&lock);
    pthread_exit(NULL);
}