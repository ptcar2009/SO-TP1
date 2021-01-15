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
#define DEADLOCK_WAIT_TIME 5000000
#define lock(a) pthread_mutex_lock(&a)
#define unlock(a) pthread_mutex_unlock(&a)

std::random_device rd;                       // obtain a random number from hardware
std::mt19937 gen(rd());                      // seed the generator
std::uniform_int_distribution<> distr(0, 2); // define the range

typedef struct pers_data
{
    pers_data(){

    };
    pers_data(std::string _name, int _thread_id) : thread_id(_thread_id), name(_name), count_forno(0), acc(false), fila(false), b4_partner(false)
    {
        pthread_cond_init(&cond, NULL);
    }
    std::string name;
    int thread_id, count_forno;
    pthread_cond_t cond;
    bool acc, fila, b4_partner;
    // bool running; <- É necessário?

} pers_data;

typedef struct monitor
{
    monitor(int _n_iterations) : n_iterations(_n_iterations)
    {
        pthread_mutex_init(&mutex_forninho, NULL);
        pthread_mutex_init(&mutex_fila, NULL);
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
    int current_front;
    void use_microwave(int);
    void release_microwave(int);
    void go_home();
    void activate_raj();
    void resolve_deadlock();
    int return_next_in_line(std::vector<int>, int);
    int check_nxt_couple(std::vector<int>, int, int);
    bool check_for_deadlock();

} monitor;

// check_nxt_couple Procura por próximo membro apto de um dos casais restantes
// check_nxt_couple Retorna -1 se membros de nenhum casal estiverem na fila
int monitor::check_nxt_couple(std::vector<int> line, int man, int wmn)
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
    if (man_in_line)
    {
        if (pers[man].acc)
        {
            if (pers[man].b4_partner)
                ret = man;
            else
                ret = wmn;
        }
        else if (other_man_in_line && pers[other_man].acc)
        {
            if (pers[other_man].b4_partner)
                ret = other_man;
            else
                ret = other_wmn;
        }
        ret = man;
    }
    // Nesse else if, necessariamente nem ela nem a outra mulher estarão acompanhadas
    else if (wmn_in_line)
        ret = wmn;
    // Valida se o outro casal se encotra na fila
    else if (other_man_in_line)
    {
        if (pers[other_man].acc)
        {
            if (pers[other_man].b4_partner)
                ret = other_man;
            else
                ret = other_wmn;
        }
    }
    else if (other_wmn_in_line)
        ret = other_wmn;

    return ret;
}

// return_next_in_line Retorna ID da próxima pessoa que deve utilizar o forno
// return_next_in_line Retorna -1 caso nenhuma pessoa esteja apta
int monitor::return_next_in_line(std::vector<int> line, int p_usingID)
{
    int nxt_in_line = -1;
    // Está acompanhadx dx parceirx e parceirx está na fila
    if (pers[p_usingID].acc)
    {
        // Homem usando o microondas
        if (p_usingID < 3 && pers[p_usingID + 5].fila)
        {
            nxt_in_line = (p_usingID + 5);
            return nxt_in_line;
        }
        // Mulher usando o microondas
        else if (p_usingID > 4 && pers[p_usingID - 5].fila)
        {
            nxt_in_line = (p_usingID - 5);
            return nxt_in_line;
        }
    }
    else
    {
        if (p_usingID == 0 || p_usingID == 5)
        {
            nxt_in_line = check_nxt_couple(line, 1, 6);
            if (nxt_in_line != -1)
                return nxt_in_line;
        }
        else if (p_usingID == 1 || p_usingID == 6)
        {
            nxt_in_line = check_nxt_couple(line, 2, 7);
            if (nxt_in_line != -1)
                return nxt_in_line;
        }
        else if (p_usingID == 2 || p_usingID == 7)
        {
            nxt_in_line = check_nxt_couple(line, 0, 5);
            if (nxt_in_line != -1)
                return nxt_in_line;
        }

        if (nxt_in_line == -1)
        {
            bool stuart_in_line = pers[3].fila;
            bool kripke_in_line = pers[4].fila;
            if (stuart_in_line)
                return 3;
            else if (kripke_in_line)
                return 4;
        }
    }
    return -1;
}

bool monitor::check_for_deadlock()
{
    std::vector<int> line;
    for (int i = 0; i < NUM_PERSONAGENS - 1; i++)
    {
        if (pers[i].fila)
            line.push_back(pers[i].thread_id);
    }
    bool sheldon = pers[0].fila;
    bool howard = pers[1].fila;
    bool leonard = pers[2].fila;
    bool amy = pers[5].fila;
    bool bernadette = pers[6].fila;
    bool penny = pers[7].fila;

    if ((amy xor sheldon) && (howard xor bernadette) && (leonard xor penny) || (sheldon && amy) && (howard && bernadette) && (leonard && penny))
        return true;

    return false;
}

void monitor::resolve_deadlock()
{
    int nxt_manID = distr(gen);

    if (pers[nxt_manID].fila)
    {
        std::cout << "Raj detectou um deadlock, liberando " << pers[nxt_manID].name << std::endl;
        pthread_cond_signal(&pers[nxt_manID].cond);
    }
    else
    {
        std::cout << "Raj detectou um deadlock, liberando " << pers[nxt_manID + 5].name << std::endl;
        pthread_cond_signal(&pers[nxt_manID + 5].cond);
    }
}

void monitor::use_microwave(int tID)
{
    lock(mutex_fila);
    std::cout << pers[tID].name << " quer usar o forno." << std::endl;
    pers[tID].b4_partner = false;
    pers[tID].fila = true;
    n_fila++;
    while (current_front != tID && !(n_fila == 1))
    {
        pthread_cond_wait(&pers[tID].cond, &mutex_fila);
    }
    unlock(mutex_fila);
    lock(mutex_forninho);
    // Incrementa contador de usos do Raj
    std::cout << pers[tID].name << " começou a esquentar algo." << std::endl;
    pers[8].count_forno += 1;
    usleep(MICROWAVE_WAIT_TIME);

}

void monitor::release_microwave(int tID)
{
    std::vector<int> pers_in_line;
    int next_persID;
    for (int i = 0; i < NUM_PERSONAGENS - 1; i++)
    {
        if (i != tID && pers[i].fila)
            pers_in_line.push_back(pers[i].thread_id);
    }

    if (pers_in_line.empty())
    {
        // Do something
    }

    lock(mutex_fila);
    std::cout << pers[tID].name << " vai comer." << std::endl;
    next_persID = monitor::return_next_in_line(pers_in_line, tID);
    current_front = next_persID;
    pers[tID].acc = false;
    pers[tID].fila = false;
    n_fila--;
    pthread_cond_signal(&pers[next_persID].cond);
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
        bool hasDeadlock = check_for_deadlock();
        if (hasDeadlock)
        {
            resolve_deadlock();
        }
        usleep(DEADLOCK_WAIT_TIME);
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
}

// Main
int main(int argc, char **argv)
{
    int nIter = atoi(argv[1]);

    Monitor = new monitor(nIter);

    for (long i = 0; i < NUM_PERSONAGENS; i++)
    {
        //int params[] = {i};
        int threadError = pthread_create(&Monitor->threads[i], NULL, go_to_work, (void *)i);

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
}