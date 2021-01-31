#include <monitor.hpp>
#include <iostream>
#include <unistd.h>
#include <random>

#define lock(a) pthread_mutex_lock(&a)
#define unlock(a) pthread_mutex_unlock(&a)
#define signal(a) pthread_cond_signal(&a)
std::random_device rd;                       // obtain a random number from hardware
std::mt19937 gen(rd());                      // seed the generator
std::uniform_int_distribution<> distr(0, 2); // define the range

Monitor::Monitor(int n_iter) : n_iterations(n_iter), deadlock(false)
{
    pthread_mutex_init(&mutex_fila, NULL);
    pthread_mutex_init(&mutex_forninho, NULL);
    pers[0] = Person("Sheldon", 0);
    pers[1] = Person("Howard", 1);
    pers[2] = Person("Leonard", 2);
    pers[3] = Person("Stuart", 3);
    pers[4] = Person("Kripke", 4);
    pers[5] = Person("Amy", 5);
    pers[6] = Person("Bernardette", 6);
    pers[7] = Person("Penny", 7);
    pers[8] = Person("Raj", 8);
}

Monitor::~Monitor()
{
    pthread_mutex_destroy(&mutex_fila);
    pthread_mutex_destroy(&mutex_forninho);
}

int Monitor::check_nxt_couple(int man, int wmn)
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
int Monitor::return_next_in_line(int p_usingID)
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

        nxt_in_line = venn();

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

int Monitor::venn()
{
    int nxt_in_line;
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
    return nxt_in_line;
}

void Monitor::resolve_deadlock()
{
    int nxt_manID = distr(gen);
    while (!(pers[nxt_manID].fila || pers[nxt_manID + 5].fila))
        nxt_manID = distr(gen);

    if (pers[nxt_manID].fila && ((pers[nxt_manID].acc && pers[nxt_manID].b4_partner) || !pers[nxt_manID].acc))
        nxt_manID = nxt_manID;
    else
        nxt_manID = nxt_manID + 5;

    std::cout << "Raj detectou um deadlock, liberando " << pers[nxt_manID].name << std::endl;
    deadlock = false;
    
    release(nxt_manID);
}

void Monitor::use_microwave(int tID)
{
    Person &cur_person = pers[tID];

    lock(mutex_fila);
    std::cout << cur_person.name << " quer usar o forno." << std::endl;
    cur_person.fila = true;

    if (tID < 3)
    {
        if (!pers[tID + 5].fila)
            cur_person.b4_partner = true;
        else
            cur_person.acc = pers[tID + 5].acc = true;
    }
    else if (tID > 4)
    {
        if (!pers[tID - 5].fila)
            cur_person.b4_partner = true;
        else
            cur_person.acc = pers[tID - 5].acc = true;
    }

    if (n_fila++ != 0)
        wait(tID);

    unlock(mutex_fila);
    lock(mutex_forninho);

    std::cout << cur_person.name << " começou a esquentar algo." << std::endl;
    cur_person.fila = false;

    // Incrementa contador de usos do Raj
    pers[8].count_forno += 1;
    usleep(MICROWAVE_WAIT_TIME);
}

void Monitor::release_microwave(int tID)
{
    Person &cur_person = pers[tID];
    int next_persID;

    lock(mutex_fila);
    n_fila--;

    std::cout << cur_person.name << " vai comer." << std::endl;

    next_persID = Monitor::return_next_in_line(tID);

    cur_person.acc = false;
    cur_person.b4_partner = false;

    if (next_persID >= 0)
        release(next_persID);

    else if (next_persID == -2)
        deadlock = true;

    release(8);
    unlock(mutex_fila);
    unlock(mutex_forninho);
}

void Monitor::go_home()
{
    usleep(HOME_WAIT_TIME);
}

void Monitor::run(void *(*function)(void *))
{
    for (long i = 0; i < NUM_PERSONAGENS; i++)
    {
        int threadError = pthread_create(&this->threads[i], NULL, function, (void *)i);
        if (threadError)
        {
            std::cout << "Unable to create thread, " << threadError << std::endl;
            exit(-1);
        }
    }

    for (int i = 0; i < NUM_PERSONAGENS; i++)
        pthread_join(this->threads[i], NULL);
}

void Monitor::activate_raj()
{
    while (pers[8].count_forno < 8 * n_iterations)
    {
        lock(mutex_fila);
        while (!deadlock && pers[8].count_forno < 8 * n_iterations)
            wait(8);

        if (deadlock)
            resolve_deadlock();

        unlock(mutex_fila);
    }
}

void Monitor::wait(int id)
{
    pthread_cond_wait(&pers[id].cond, &mutex_fila);
}

void Monitor::release(int id)
{
    pthread_cond_signal(&pers[id].cond);
}