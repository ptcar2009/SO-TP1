#include <person.hpp>
Person::Person()
{
}

Person::Person(std::string _name, int _thread_id)
    : thread_id(_thread_id),
      name(_name),
      count_forno(0),
      acc(false),
      fila(false),
      b4_partner(false)
{
    pthread_cond_init(&cond, NULL);
}

Person::~Person() {
    pthread_cond_destroy(&cond);
}