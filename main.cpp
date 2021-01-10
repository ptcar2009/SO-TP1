#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds

#define NUM_PERSONAGENS 9
#define lock(a) pthread_mutex_lock(a)

typedef struct monitor
{
    monitor()
    {
        // Initialize mutex
        if (pthread_mutex_init(&mutex, NULL) != 0)
        {
            std::cout << "Mutex init failed" << std::endl;
            return;
        }

        for (int i = 0; i < NUM_PERSONAGENS; i++)
        {
            int threadError = pthread_create(&threads[i], NULL, ir_ao_trabalho, &i);

            if (threadError)
            {
                std::cout << "Unable to create thread, " << threadError << std::endl;
                exit(-1);
            }
        }

        for (int i = 0; i < NUM_PERSONAGENS; i++)
        {
            pthread_join(threads[i], NULL);
        }

        pthread_mutex_destroy(&mutex);
        pthread_exit(NULL);
    }
    static thread_data pers[9];
    static pthread_t threads[9];
    static pthread_mutex_t mutex;

    static void useMicrowave(int);
    static void releaseMicrowave(int);

} monitor;

typedef struct thread_data
{
    std::string name;
    int thread_id;
    bool acc;
    int count_forno;
    pthread_cond_t cond;
    bool running, fila;

} thread_data;

void monitor::useMicrowave(int tID)
{
    std::cout << pers[tID].name << " quer usar o forno." << std::endl;
    if (tID < 3 && pers[tID + 5].fila)
        pers[tID + 5].acc = pers[tID].acc = true;

    if (tID > 4 && tID < 8 && pers[tID - 5].fila)
        pers[tID - 5].acc = pers[tID].acc = true;

    pers[tID].fila = true;

    if (tID < 3)
    {
        int bp = (tID + 1) % 3;
        int sp = (tID - 1 >= 0 ? tID - 1 : 2);
        if (pers[tID].acc)
        {
            if ((pers[bp].acc && pers[bp].fila) || pers[bp + 5].acc && pers[bp + 5].fila)
                pthread_cond_wait(&pers[tID].cond, &mutex);
        }
        else
        {
            if (((pers[sp].acc && pers[sp].fila) || (pers[sp + 5].acc && pers[sp + 5].fila)) || (pers[bp].fila || pers[bp + 5].fila))
                pthread_cond_wait(&pers[tID].cond, &mutex);
        }
    }
    else if (tID == 3)
    {
        bool cond = false;
        for (int i = 0; i < 3; cond |= pers[i++].fila)
            ;
        for (int i = 5; i < 8; cond |= pers[i++].fila)
            ;
        if (cond)
            pthread_cond_wait(&pers[tID].cond, &mutex);
    }
    else if (tID == 4)
    {
        bool cond = false;
        for (int i = 0; i < 4; cond |= pers[i++].fila)
            ;
        for (int i = 5; i < 8; cond |= pers[i++].fila)
            ;
        if (cond)
            pthread_cond_wait(&pers[tID].cond, &mutex);
    }
    else if (tID < 8)
    {
        int bp = (tID - 4) % 3 + 5;
        int sp = (tID - 6 >= 0 ? tID - 1 : 2) + 5;
        if (pers[tID].acc)
        {
            if ((pers[bp].acc && pers[bp].fila) || pers[bp - 5].acc && pers[bp - 5].fila)
                pthread_cond_wait(&pers[tID].cond, &mutex);
        }
        else
        {
            if (((pers[sp].acc && pers[sp].fila) || (pers[sp - 5].acc && pers[sp - 5].fila)) || (pers[bp].fila || pers[bp - 5].fila))
                pthread_cond_wait(&pers[tID].cond, &mutex);
        }
    }
    pthread_mutex_lock(&mutex);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void monitor::releaseMicrowave(int tID)
{
}

void *ir_ao_trabalho(void *tID)
{
    int ID = *((int *)tID);
    int nIter = (&ID)[1];
    monitor::useMicrowave(ID);
    monitor::releaseMicrowave(ID);
}

// Main
int main(int argc, char **argv)
{
}