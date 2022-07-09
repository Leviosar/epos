#include <utility/ostream.h>
#include <architecture/rv64/rv64_cpu.h>
#include <machine/riscv/riscv_timer.h>
#include <machine/riscv/riscv_ic.h>

#include <machine/display.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

const int iterations = 10;

Thread * workers[10];

OStream cout;

int worker(int i);

int main()
{
    cout << "Starting scheduling tests with " << CPU::cores() << " cores" << endl;
    cout << endl;

    for (auto i = 0; i < iterations; i++) {
        workers[i] = new Thread(&worker, i);
        cout << "Created thread " << i << endl;
    }

    for (auto i = 0; i < iterations; i++) {
        int work = workers[i]->join();
        cout << "Thread " << i << " returned " << work << endl;
    }

    return 0;
}

int worker(int i) {
    cout << "Core " << CPU::id() << " is saying hello world: " << i << "!" << endl;
    
    int j = 0;

    for (auto i = 0; i < 100000000; i++) {
        j++;
    }

    return j + i;
}