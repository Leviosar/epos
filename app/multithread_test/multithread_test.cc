#include <utility/ostream.h>
#include <architecture/rv64/rv64_cpu.h>
#include <machine/riscv/riscv_timer.h>
#include <machine/riscv/riscv_ic.h>

#include <machine/display.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

#define THREAD_COUNT 8

Thread * workers[8];

OStream cout;

int worker(int i);

int main()
{
    cout << "Starting scheduling tests with " << CPU::cores() << " cores" << endl;
    cout << endl;

    for (auto i = 0; i < THREAD_COUNT; i++) {
        workers[i] = new Thread(&worker, i);
        cout << "Created thread " << i << endl;
    }

    // Each time one of the words is swapped from a queue to another
    // a WRN is raised, we kept warnings ON for test purposes
    for (auto i = 0; i < THREAD_COUNT; i++) {
        int work = workers[i]->join();
        cout << "Thread " << i << " returned from core " << work << " with priority " << workers[i]->criterion() << endl;
    }

    return 0;
}

// Worker with dummy operations that take long enough to be preempted
int worker(int i) {
    int j = 0;

    int core = CPU::id();

    for (auto i = 0; i < 10000000; i++) {
        j++;
    }

    return core;
}