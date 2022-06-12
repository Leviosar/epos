#include <machine/display.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

#define THREAD_COUNT 20

Thread * workers[THREAD_COUNT];

OStream cout;

int cpu_work() {
    cout << "Starting CPU bound thread execution" << endl;

    unsigned int dummy[10000000];

    unsigned int sum = 0;

    size_t i = 0;
    
    for (i = 0; i < 10000000; i++) {
        sum += dummy[i];
    }

    cout << "Finishing CPU bound thread execution" << endl;

    return sum;
}

int io_work() {
    cout << "Starting IO bound thread execution" << endl;

    Delay(6000);
    
    cout << "Finishing IO bound thread execution" << endl;

    return 0;
}

int main()
{
    cout << "Starting sched test!" << endl;
    
    for (auto i = 0; i < THREAD_COUNT; i++) {
        if (i % 2 == 0) {
            cout << "Creating IO bound thread worker" << endl;
            workers[i] = new Thread(&io_work);
        } else {
            cout << "Creating CPU bound thread worker" << endl;
            workers[i] = new Thread(&cpu_work);
        }
    }

    for (auto i = 0; i < THREAD_COUNT; i++) {
        workers[i]->join();
    }

    cout << "Starting sched end!" << endl;

    return 0;
}