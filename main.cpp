#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <random>

#ifndef NUM_PERSONAGENS
#define NUM_PERSONAGENS 9
#endif
#define MAX_PERSONAGENS 9
#define HOME_WAIT_TIME 600000
#define MICROWAVE_WAIT_TIME 1000000
#define DEADLOCK_WAIT_TIME 500000
#define lock(a) pthread_mutex_lock(&a)
#define unlock(a) pthread_mutex_unlock(&a)

std::random_device rd;                       // obtain a random number from hardware
std::mt19937 gen(rd());                      // seed the generator
std::uniform_int_distribution<> distr(0, 2); // define the range

typedef struct pers_data
{
    pers_data(){

    };
    pers_data(std::string _name, int _thread_id)
        : thread_id(_thread_id),
          name(_name),
          count_forno(0),
          acc(false),
          fila(false),
          b4_partner(false),
          can_go(false)
    {
        pthread_cond_init(&cond, NULL);
    }
    std::string name;
    int thread_id, count_forno;
    pthread_cond_t cond;
    bool acc, fila, b4_partner, can_go;
    // bool running; <- É necessário?

    ~pers_data()
    {
        pthread_cond_destroy(&this->cond);
    }

} pers_data;

typedef struct monitor
{

    monitor(int _n_iterations) : n_iterations(_n_iterations), deadlock(false)
    {
        pthread_mutex_init(&mutex_forninho, NULL);
        pthread_mutex_init(&mutex_fila, NULL);
        current_front = -1;
        pers[0] = pers_data("Sheldon", 0);
        pers[1] = pers_data("Howard", 1);
        pers[2] = pers_data("Leonard", 2);
        pers[3] = pers_data("Stuart", 3);
        pers[4] = pers_data("Kripke", 4);
        pers[5] = pers_data("Amy", 5);
        pers[6] = pers_data("Bernardette", 6);
        pers[7] = pers_data("Penny", 7);
        pers[8] = pers_data("Raj", 8);
    }
    int n_fila = 0;
    pers_data pers[MAX_PERSONAGENS];
    pthread_t threads[NUM_PERSONAGENS];
    pthread_mutex_t mutex_forninho;
    pthread_mutex_t mutex_fila;
    int n_iterations;
    bool deadlock;
    int current_front;
    void use_microwave(int);
    void release_microwave(int);
    void go_home();
    void activate_raj();
    void resolve_deadlock();
    int return_next_in_line(int);
    int check_nxt_couple(int, int);
    bool check_for_deadlock();

} monitor;

// check_nxt_couple Procura por próximo membro apto de um dos casais restantes
// check_nxt_couple Retorna -1 se membros de nenhum casal estiverem na fila
int monitor::check_nxt_couple(int man, int wmn)
{

    int other_man = (man + 1) % 3;
    int other_wmn = other_man + 5;
    bool man_in_line = pers[man].fila;
    bool wmn_in_line = pers[wmn].fila;
    bool other_man_in_line = pers[other_man].fila;
    bool other_wmn_in_line = pers[other_wmn].fila;

    int ret = -1;
    // Casos abordados: 1) homem na fila (acompanhado ou não) e
    //                  2) outro homem na fila (acompanhado ou não)
    if (man_in_line || wmn_in_line)
    {
        if (pers[man].acc && pers[wmn].acc)
            ret = pers[man].b4_partner ? man : wmn;

        else if (other_man_in_line && pers[other_man].acc)
            ret = pers[other_man].b4_partner ? other_man : other_wmn;

        else
            ret = man_in_line ? man : wmn;
    }
    // Nesse else if, necessariamente nem ela nem a outra mulher estarão acompanhadas
    // Valida se o outro casal se encotra na fila
    else if (other_man_in_line || other_wmn_in_line)
    {
        if (pers[other_man].acc && pers[other_wmn].acc)
            ret = pers[other_man].b4_partner ? other_man : other_wmn;
        else
            ret = other_man_in_line ? other_man : other_wmn;
    }
    return ret;
}

// return_next_in_line Retorna ID da próxima pessoa que deve utilizar o forno
// return_next_in_line Retorna -1 caso nenhuma pessoa esteja apta
int monitor::return_next_in_line(int p_usingID)
{
    int nxt_in_line = -1;
    // Está acompanhadx dx parceirx e parceirx está na fila
    if (pers[p_usingID].acc && pers[p_usingID].b4_partner)
    {
        // Homem usando o microondas
        if (p_usingID < 3 && pers[p_usingID + 5].fila)
            nxt_in_line = (p_usingID + 5);

        // Mulher usando o microondas
        else if (p_usingID > 4 && pers[p_usingID - 5].fila)
            nxt_in_line = (p_usingID - 5);
    }
    else
    {

        int onesix = check_nxt_couple(1, 6);
        int twoseven = check_nxt_couple(2, 7);
        int zerofive = check_nxt_couple(0, 5);

        if (twoseven == zerofive)
            nxt_in_line = twoseven;

        else if (zerofive == onesix)
            nxt_in_line = onesix;

        else if (twoseven == onesix)
            nxt_in_line = twoseven;

        else if (twoseven != -1 && zerofive != -1 && onesix != -1)
            nxt_in_line = -2;

        if (nxt_in_line == -1)
        {
            bool stuart_in_line = pers[3].fila;
            bool kripke_in_line = pers[4].fila;

            if (stuart_in_line)
                return 3;
            if (kripke_in_line)
                return 4;
        }
    }
    return nxt_in_line;
}
void monitor::resolve_deadlock()
{
    int nxt_manID = distr(gen);
    while (!(pers[nxt_manID].fila || pers[nxt_manID + 5].fila))
        nxt_manID = distr(gen);

    if (pers[nxt_manID].fila && ((pers[nxt_manID].acc && pers[nxt_manID].b4_partner) || !pers[nxt_manID].acc))
        nxt_manID = nxt_manID;
    else
        nxt_manID = nxt_manID + 5;

    std::cout << "Raj detectou um deadlock, liberando " << pers[nxt_manID].name << std::endl;
    pers[nxt_manID].can_go = true;
    deadlock = false;
    pthread_cond_signal(&pers[nxt_manID].cond);
}

void monitor::use_microwave(int tID)
{
    lock(mutex_fila);
    std::cout << pers[tID].name << " quer usar o forno." << std::endl;
    pers[tID].fila = true;
    if (n_fila++ == 0)
        current_front = tID;

    if (tID < 3)
    {
        if (!pers[tID + 5].fila)
            pers[tID].b4_partner = true;
        else
            pers[tID].acc = pers[tID + 5].acc = true;
    }
    else if (tID > 4)
    {
        if (!pers[tID - 5].fila)
            pers[tID].b4_partner = true;
        else
            pers[tID].acc = pers[tID - 5].acc = true;
    }

    while (current_front != tID and !pers[tID].can_go)
    {
        pthread_cond_wait(&pers[tID].cond, &mutex_fila);
    }

    unlock(mutex_fila);
    lock(mutex_forninho);

    std::cout << pers[tID].name << " começou a esquentar algo." << std::endl;
    pers[tID].fila = false;
    pers[tID].can_go = false;

    // Incrementa contador de usos do Raj
    pers[8].count_forno += 1;
    usleep(MICROWAVE_WAIT_TIME);
}

void monitor::release_microwave(int tID)
{
    int next_persID;

    lock(mutex_fila);
    n_fila--;

    std::cout << pers[tID].name << " vai comer." << std::endl;

    next_persID = monitor::return_next_in_line(tID);

    pers[tID].acc = false;
    pers[tID].b4_partner = false;
    
    current_front = next_persID;

    if (next_persID >= 0)
        pthread_cond_signal(&pers[next_persID].cond);

    else if (next_persID == -2)
        deadlock = true;

    pthread_cond_signal(&pers[8].cond);
    unlock(mutex_fila);
    unlock(mutex_forninho);
}

void monitor::go_home()
{
    usleep(HOME_WAIT_TIME);
}

void monitor::activate_raj()
{
    while (pers[8].count_forno < 8 * n_iterations)
    {
        lock(mutex_fila);
        while (!deadlock && pers[8].count_forno < 8 * n_iterations)
            pthread_cond_wait(&pers[8].cond, &mutex_fila);

        if (deadlock)
            resolve_deadlock();

        unlock(mutex_fila);
    }
}
monitor *Monitor;

void *go_to_work(void *tID)
{
    long ID = long(tID);
    if (ID != 8)
    {
        for (int i = 0; i < Monitor->n_iterations; i++)
        {
            if (i > 0)
                std::cout << Monitor->pers[ID].name << " voltou para o trabalho." << std::endl;
            Monitor->use_microwave(ID);
            Monitor->release_microwave(ID);
            Monitor->go_home();
        }
    }
    else
    {
        Monitor->activate_raj();
    }
    return 0;
}

// Main
int main(int argc, char **argv)
{
    int nIter = /* atoi(argv[1]) */ 3;

    Monitor = new monitor(nIter);
    long order[] = {3, 4, 0, 1, 2, 5, 6, 7, 8};

    for (long i = 0; i < NUM_PERSONAGENS; i++)
    {
        //int params[] = {i};
        int threadError = pthread_create(&Monitor->threads[order[i]], NULL, go_to_work, (void *)order[i]);
        // sleep(1);
        if (threadError)
        {
            std::cout << "Unable to create thread, " << threadError << std::endl;
            exit(-1);
        }
    }

    for (int i = 0; i < NUM_PERSONAGENS; i++)
    {
        pthread_join(Monitor->threads[i], NULL);
    }

    pthread_mutex_destroy(&Monitor->mutex_forninho);
    pthread_mutex_destroy(&Monitor->mutex_fila);
    pthread_exit(NULL);
    delete (Monitor);
}