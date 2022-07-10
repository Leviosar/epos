// EPOS Scheduler Component Declarations

#ifndef __scheduler_h
#define __scheduler_h

#include <architecture/cpu.h>
#include <architecture/pmu.h>
#include <architecture/tsc.h>
#include <utility/scheduling.h>
#include <utility/math.h>
#include <utility/convert.h>

__BEGIN_SYS

// All scheduling criteria, or disciplines, must define operator int() with
// the semantics of returning the desired order of a given object within the
// scheduling list
class Scheduling_Criterion_Common
{
    friend class _SYS::Thread;
    friend class _SYS::Periodic_Thread;
    friend class _SYS::RT_Thread;
    friend class _SYS::Clerk<System>;         // for _statistics

public:
    // Priorities
    // enum : int {
    //     MAIN   = 0,
    //     HIGH   = 1,
    //     NORMAL = (unsigned(1) << (sizeof(int) * 8 - 1)) - 3,
    //     LOW    = (unsigned(1) << (sizeof(int) * 8 - 1)) - 2,
    //     IDLE   = (unsigned(1) << (sizeof(int) * 8 - 1)) - 1
    // };
    enum : int {
        MAIN   = 0,
        HIGH   = 100,
        NORMAL = 120,
        LOW    = 130,
        IDLE   = 281
    };

    // Constructor helpers
    enum : unsigned int {
        SAME        = 0,
        NOW         = 0,
        UNKNOWN     = 0,
        ANY         = -1U
    };

    // Policy types
    enum : int {
        PERIODIC    = HIGH,
        APERIODIC   = NORMAL,
        SPORADIC    = NORMAL
    };

    // Policy traits
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = true;
    static const bool collecting = false;
    static const bool charging = false;
    static const bool awarding = false;
    static const bool migrating = false;
    static const bool switching = false;
    static const bool track_idle = false;
    static const bool task_wide = false;
    static const bool cpu_wide = false;
    static const bool system_wide = false;
    static const unsigned int QUEUES = 1;
    static const unsigned int current_queue = 1;

    // Runtime Statistics (for policies that don't use any; that´s why its a union)
    union Statistics {
        // Thread Execution Time
        TSC::Time_Stamp thread_execution_time;  // accumulated thread execution time
        TSC::Time_Stamp last_thread_dispatch;   // time stamp of last dispatch

        // Deadline Miss count - Used By Clerk
        Alarm * alarm_times;                    // pointer to RT_Thread private alarm (for monitoring purposes)
        unsigned int finished_jobs;             // number of finished jobs given by the number of times alarm->p() was called for this thread
        unsigned int missed_deadlines;          // number of missed deadlines given by the number of finished jobs (finished_jobs) minus the number of dispatched jobs (alarm_times->times)

        // CPU Execution Time (capture ts)
        static TSC::Time_Stamp _cpu_time[Traits<Build>::CPUS];              // accumulated CPU time in the current hyperperiod for each CPU
        static TSC::Time_Stamp _last_dispatch_time[Traits<Build>::CPUS];    // time Stamp of last dispatch in each CPU
        static TSC::Time_Stamp _last_activation_time;                       // global time stamp of the last heuristic activation
    };

protected:
    Scheduling_Criterion_Common() {}

public:
    const Microsecond period() { return 0;}
    void period(const Microsecond & p) {}

    unsigned int queue() const { return 0; }
    void queue(unsigned int q) {}

    bool update() { return false; }

    bool collect(bool end = false) { return false; }
    bool charge(bool end = false) { return true; }
    bool award(bool end = false) { return true; }
    bool _switch(bool end = false) { return false; }

    volatile Statistics & statistics() { return _statistics; }

    static void init() {}

protected:
    Statistics _statistics;
};

// Priority (static and dynamic)
class Priority: public Scheduling_Criterion_Common
{
    friend class _SYS::Thread;
    friend class _SYS::Periodic_Thread;
    friend class _SYS::RT_Thread;

public:
    template <typename ... Tn>
    Priority(int p = NORMAL, Tn & ... an): _priority(p) {}

    operator const volatile int() const volatile { return _priority; }

protected:
    volatile int _priority;
};

// Round-Robin
class RR: public Priority
{
public:
    static const bool timed = true;
    static const bool dynamic = false;
    static const bool preemptive = true;

public:
    template <typename ... Tn>
    RR(int p = NORMAL, Tn & ... an): Priority(p) {}
};

// Global Round-Robin
class GRR: public RR
{
public:
    static const unsigned int HEADS = Traits<Machine>::CPUS;

public:
    template <typename ... Tn>
    GRR(int p = NORMAL, Tn & ... an): RR(p) {}

    static unsigned int current_head() { return CPU::id(); }
};

// The Chosen One
/*
    The Chosen One is a constant time scheduling algorithm based on the old O(1) linux scheduler

    It uses two queues: active and expired (emulated here by a multiplier) for preventing starvation,
    putting newly added threads into the active queue and switching them after one QUANTUM of execution.

    In the original O(1), if the active queue is empty, then the pointers for the queues are swapped
    and the expired becomes the new active and a new cycle of executions will begin.

    In The Chosen One we opted for using a single queue with an offset multiplier for its first implementation,
    when a thread uses all its CPU time, we switch the current_queue. All members of queue 01 will have
    a higher priority than any member of queue 02.

    We've added a new 'switching' flag on policies, and a new _switch method which is called every time
    a schedule timer interruption is triggered for a thread.

    In a scenario where new threads are continuosly added to the ready queue, this scheduler can suffer
    from starvation, but so does O(1) and thats one of the reasons why it was replaced for CFS.
*/
class TCO: public RR
{
public:
    static const unsigned int HEADS = Traits<Machine>::CPUS;
    static const bool switching = true;
public:
    template <typename ... Tn>
    TCO(int p = NORMAL, Tn & ... an): RR(p), current_queue{1} { }

    unsigned int current_queue;

    // IDLE and MAIN threads are always kept in 01 queue, thus we do not need
    // to check for specific cases in the casting operator
    operator const volatile int() const volatile {
        return _priority * current_queue;
    }

    static unsigned int current_head() { return CPU::id(); }

    bool _switch() {
        // Never change MAIN or IDLE queues
        if (_priority == MAIN || _priority == IDLE) return false;
        
        current_queue = current_queue == 1 ? 2 : 1;

        return true;
    }
};

// Non-preemptive dynamic priority scheduler that benefits I/O-bound threads
// We're assuming by the EPOS design that a thread going into "sleep" is
// being interrupted for I/O operations.
class IOB: public Priority
{
public:
    static const bool timed = true;
    static const bool dynamic = true;
    static const bool preemptive = false;
    static const bool collecting = true;

public:
    template <typename ... Tn>
    
    IOB(int p = NORMAL, Tn & ... an) : Priority(p), _sleep_counter{0} {}

    unsigned int _sleep_counter;

    bool collect(bool end = false) {
        _sleep_counter++;
        return false;
    }

    operator const volatile int() const volatile {
        if (_priority == IDLE) return IDLE;

        if (_priority - _sleep_counter < 1) return HIGH;

        return _priority - _sleep_counter;
    }
};

// Time-preemptive dynamic priority scheduler that benefits I/O-bound threads
// We're assuming by the EPOS design that a thread going into "sleep" is
// being interrupted for I/O operations.
class TIOB: public Priority
{
public:
    static const bool timed = true;
    static const bool dynamic = true;
    static const bool preemptive = true;
    static const bool collecting = true;

public:
    template <typename ... Tn>
    
    TIOB(int p = NORMAL, Tn & ... an) : Priority(p), _sleep_counter{0} {}

    unsigned int _sleep_counter;

    bool collect(bool end = false) {
        _sleep_counter++;
        return false;
    }

    operator const volatile int() const volatile {
        if (_priority == IDLE) return IDLE;

        if (_priority - _sleep_counter < 1) return HIGH;

        return _priority - _sleep_counter;
    }
};

// First-Come, First-Served (FIFO)
class FCFS: public Priority
{
public:
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = false;

public:
    template <typename ... Tn>
    FCFS(int p = NORMAL, Tn & ... an);
};

__END_SYS

__BEGIN_UTIL

// Scheduling Queues
template<typename T>
class Scheduling_Queue<T, GRR>:
public Multihead_Scheduling_List<T> {};

template<typename T>
class Scheduling_Queue<T, TCO>:
public Multihead_Scheduling_List<T> {};

__END_UTIL

#endif
