#include <monitor.hpp>
#include <iostream>
Monitor* monitor;
void *go_to_work(void *tID)
{
    long ID = long(tID);
    if (ID != 8)
    {
        for (int i = 0; i < monitor->n_iterations; i++)
        {
            if (i > 0)
                std::cout << monitor->pers[ID].name << " voltou para o trabalho." << std::endl;
            monitor->use_microwave(ID);
            monitor->release_microwave(ID);
            monitor->go_home();
        }
    }
    else
    {
        monitor->activate_raj();
    }
    return 0;
}

// Main
int main(int argc, char **argv)
{
    int nIter = atoi(argv[1]);
    monitor = new Monitor(nIter);
    monitor->run(go_to_work);
    delete (monitor);
}