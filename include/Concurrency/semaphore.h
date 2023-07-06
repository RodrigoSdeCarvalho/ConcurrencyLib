#ifndef semaphore_h
#define semaphore_h

#include "Concurrency/cpu.h"
#include "Concurrency/thread.h"
#include "Concurrency/traits.h"
#include "Concurrency/debug.h"
#include "Concurrency/list.h"

__BEGIN_API

class Semaphore
{
public:
    typedef Ordered_List<Thread> Asleep_Queue;

    Semaphore(int v = 1) : _value(v) {}
    ~Semaphore();

    void p();
    void v();

private:
    // Atomic operations
    int finc(volatile int & number);
    int fdec(volatile int & number);

    // Thread operations
    void sleep();
    void wakeup(bool reschedule = true);
    void wakeup_all();

private:
    Asleep_Queue _asleep;
    volatile int _value;
};

__END_API

#endif