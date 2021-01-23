#if !defined(PERSON_HPP)
#define PERSON_HPP
#include <string>
class Person
{
public:
    Person();
    Person(std::string _name, int _thread_id);
    std::string name;
    int thread_id, count_forno;
    pthread_cond_t cond;
    bool acc, fila, b4_partner;
    // bool running; <- É necessário?

    ~Person();

};

#endif // PERSON_HPP
