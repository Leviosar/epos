#include <machine/display.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

#define THREAD_COUNT 20

Thread * workers[THREAD_COUNT];

OStream cout;

int cpu_work(int index) {
    cout << "Starting CPU bound thread execution with priority of " << (int) workers[index]->priority() << endl;

    unsigned int dummy[10000000];

    unsigned int sum = 0;

    size_t i = 0;
    
    for (i = 0; i < 10000000; i++) {
        sum += dummy[i];
    }

    cout << "Finishing CPU bound thread execution with priority of " << (int) workers[index]->priority() << endl;

    return sum;
}

int io_work(int index) {
    cout << "Starting IO bound thread execution with priority of " << (int) workers[index]->priority() << endl;

    Delay(6000);
    
    cout << "Finishing IO bound thread execution with priority of  " << (int) workers[index]->priority() << endl;

    return 0;
}

int main()
{
    cout << "Starting sched test!" << endl;
    
    for (auto i = 0; i < THREAD_COUNT; i++) {
        if (i % 2 == 0) {
            cout << "Creating IO bound thread worker" << endl;
            workers[i] = new Thread(&io_work, i);
        } else {
            cout << "Creating CPU bound thread worker" << endl;
            workers[i] = new Thread(&cpu_work, i);
        }
    }

    for (auto i = 0; i < THREAD_COUNT; i++) {
        workers[i]->join();
    }

    cout << "Starting sched end!" << endl;

    return 0;
}