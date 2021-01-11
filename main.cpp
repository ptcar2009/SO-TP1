#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <random>

#define NUM_PERSONAGENS 9
#define HOME_WAIT_TIME 6000000
#define MICROWAVE_WAIT_TIME 1000000
#define DEADLOCK_WAIT_TIME 5000000
#define lock(a) pthread_mutex_lock(a)

std::random_device rd; // obtain a random number from hardware
std::mt19937 gen(rd()); // seed the generator
std::uniform_int_distribution<> distr(0, 2); // define the range

typedef struct pers_data
{
    std::string name;
    int thread_id, count_forno;
    pthread_cond_t cond;
    bool acc, fila, b4_partner;
    // bool running; <- É necessário?

} pers_data;

typedef struct monitor
{
    pers_data pers[9];
    pthread_t threads[9];
    pthread_mutex_t mutex;
    int n_iterations;

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
    bool man_in_line = std::find(line.begin(), line.end(), man) != line.end();
    bool wmn_in_line = std::find(line.begin(), line.end(), wmn) != line.end();
    bool other_man_in_line = std::find(line.begin(), line.end(), other_man) != line.end();
    bool other_wmn_in_line = std::find(line.begin(), line.end(), other_wmn) != line.end();
    // Casos abordados: 1) homem na fila (acompanhado ou não) e
    //                  2) outro homem na fila (acompanhado ou não)
    if (man_in_line)
    {
        if (pers[man].acc)
        {
            if (pers[man].b4_partner)
                return man;
            else
                return wmn;                    
        }
        else if (other_man_in_line && pers[other_man].acc)
        {
            if (pers[other_man].b4_partner)
                return other_man;
            else
                return other_wmn;
        }
        return man;
    }
    // Nesse else if, necessariamente nem ela nem a outra mulher estarão acompanhadas
    else if (wmn_in_line)
        return wmn;
    // Valida se o outro casal se encotra na fila
    else if (other_man_in_line)
    {
        if (pers[other_man].acc)
        {
            if (pers[other_man].b4_partner)
                return other_man;
            else
                return other_wmn;
        }
    }
    else if (other_wmn_in_line)
        return other_wmn;

    return -1;
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
            nxt_in_line =  (p_usingID + 5);
            return nxt_in_line;
        }
        // Mulher usando o microondas
        else if (p_usingID > 4 && pers[p_usingID - 5].fila)
        {
            nxt_in_line =  (p_usingID - 5);
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
    for(int i = 0; i < NUM_PERSONAGENS - 1; i++)
    {
        if (pers[i].fila)
            line.push_back(pers[i].thread_id);
    }
    bool sheldon = std::find(line.begin(), line.end(), 0) != line.end();
    bool howard = std::find(line.begin(), line.end(), 1) != line.end();
    bool leonard = std::find(line.begin(), line.end(), 2) != line.end();
    bool amy = std::find(line.begin(), line.end(), 5) != line.end();
    bool bernadette = std::find(line.begin(), line.end(), 6) != line.end();
    bool penny = std::find(line.begin(), line.end(), 7) != line.end();

    if ((sheldon||amy) && (howard||bernadette) && (leonard||penny))
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
    else {
        std::cout << "Raj detectou um deadlock, liberando " << pers[nxt_manID + 5].name << std::endl;
        pthread_cond_signal(&pers[nxt_manID + 5].cond);
    }
}

void monitor::use_microwave(int tID)
{
    std::cout << pers[tID].name << " quer usar o forno." << std::endl;
    pers[tID].b4_partner = false;
    if (tID < 3 && pers[tID + 5].fila)
    {
        pers[tID + 5].b4_partner = true;
        pers[tID + 5].acc = pers[tID].acc = true;
    }   
    else if (tID > 4 && tID < 8 && pers[tID - 5].fila)
    {
        pers[tID - 5].b4_partner = true;
        pers[tID - 5].acc = pers[tID].acc = true;
    }

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
    // Incrementa contador de usos do Raj
    pers[9].count_forno += 1;
    usleep(MICROWAVE_WAIT_TIME);
}

void monitor::release_microwave(int tID)
{
    std::vector<int> pers_in_line;
    int next_persID;
    for(int i = 0; i < NUM_PERSONAGENS - 1; i++)
    {
        if (i != tID && pers[i].fila)
            pers_in_line.push_back(pers[i].thread_id);
    }
    
    if (pers_in_line.empty())
    {
        // Do something
    }

    next_persID = monitor::return_next_in_line(pers_in_line, tID);

    std::cout << pers[tID].name << " vai comer." << std::endl;
    pers[tID].acc = false;
    pers[tID].fila = false;
    pthread_cond_signal(&pers[next_persID].cond);
}

void monitor::go_home()
{
    usleep(HOME_WAIT_TIME);
}

void monitor::activate_raj()
{
    while (pers[8].count_forno < 8*n_iterations)
    {
        bool hasDeadlock = check_for_deadlock();
        if (hasDeadlock)
        {
            resolve_deadlock();
        }
        usleep(DEADLOCK_WAIT_TIME);
    }
}

void *go_to_work(void *tID)
{
    int ID = *((int *)tID);
    int nIter = (&ID)[1];
    if (ID != 8)
    {
        for (int i=0; i<monitor::n_iterations; i++)
        {
            if (i > 0)
                std::cout << monitor::pers[ID].name << " voltou para o trabalho." << std::endl;
            monitor::use_microwave(ID);
            monitor::release_microwave(ID);
            monitor::go_home();
        }
    }
    else
    {
        monitor::activate_raj();
    }
}

// Main
int main(int argc, char **argv)
{
    monitor Monitor;
    // Initialize mutex
    if (pthread_mutex_init(&Monitor.mutex, NULL) != 0)
    {
        std::cout << "Mutex init failed" << std::endl;
        return -1;
    }

    for (int i = 0; i < NUM_PERSONAGENS; i++)
    {
        int threadError = pthread_create(&Monitor.threads[i], NULL, go_to_work, &i);

        if (threadError)
        {
            std::cout << "Unable to create thread, " << threadError << std::endl;
            exit(-1);
        }
    }

    for (int i = 0; i < NUM_PERSONAGENS - 1; i++)
    {
        pthread_join(Monitor.threads[i], NULL);
    }

    pthread_mutex_destroy(&Monitor.mutex);
    pthread_exit(NULL);
}