#if !defined(MONITOR_HPP)
#define MONITOR_HPP
#include <pthread.h>
#include <person.hpp>
#ifndef NUM_PERSONAGENS
#define NUM_PERSONAGENS 9
#endif
#define MAX_PERSONAGENS 9
#define HOME_WAIT_TIME 600000
#define MICROWAVE_WAIT_TIME 1000000
#define DEADLOCK_WAIT_TIME 500000
class Monitor
{
public:
    Monitor(int _n_iterations);
    ~Monitor();
    void use_microwave(int);
    void release_microwave(int);
    void activate_raj();
    void go_home();
    Person pers[MAX_PERSONAGENS];
    void run(void* (*thread_f)(void*));
    int n_iterations;
private:
    int n_fila = 0;
    bool deadlock;
    pthread_t threads[NUM_PERSONAGENS];
    pthread_mutex_t mutex_forninho;
    pthread_mutex_t mutex_fila;
    void resolve_deadlock();
    int return_next_in_line(int);
    int check_nxt_couple(int, int);
    int venn();
    void wait(int);
    void release(int);
};

#endif // MONITOR_HPP
