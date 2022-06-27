#include <utility/ostream.h>
#include <architecture/rv64/rv64_cpu.h>
#include <machine/riscv/riscv_timer.h>
#include <machine/riscv/riscv_ic.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "Starting configuration tests with " << CPU::cores() << "cores" << endl;
    cout << endl;

    cout << "Current core id: " << CPU::id() << endl;

    cout << endl;

    cout << "------------- Timer -------------" << endl;
    cout << endl;
    cout << "Checkin mtimecmp:" << endl;

    auto mtimecmp = Timer::reg(CLINT::MTIMECMP);
    cout << "Current mtimecmp is: " << mtimecmp << endl;
    
    Timer::config(1UL);
    
    auto new_mtimecmp = Timer::reg(CLINT::MTIMECMP);
    cout << "Current mtimecmp is: " << new_mtimecmp << endl;
    
    assert(new_mtimecmp > mtimecmp + 1UL);

    cout << endl;

    cout << "------------- Stack -------------" << endl;
    auto stack_base = Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * (CPU::id() + 1) - sizeof(long);
    auto stack_top = Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * (CPU::id() + 2) - sizeof(long);
    auto stack_pointer = CPU::sp();
    cout << "Stack base for this core is at " << hex << stack_base << endl;
    cout << "Stack top for this core is at " << hex << stack_top << endl;
    cout << "Stack pointer is at " << hex << stack_pointer << endl;

    return 0;
}