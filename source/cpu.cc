#include "Concurrency/cpu.h"
#include <iostream>

__BEGIN_API

using namespace std;

void CPU::Context::save()
{
    ucontext_t *contextToSavePtr = &this->_context;
    getcontext(contextToSavePtr);
}

void CPU::Context::load()
{
    ucontext_t *contextToLoadPtr = &this->_context;
    setcontext(contextToLoadPtr);

}

CPU::Context::~Context()
{
    if (this->_stack) // Se o valor apontado por _stack for diferente de 0, esse valor não será destruído no destructor padrão.
                           // Embora o ponteiro seja destruído, o valor apontado não é. 
    {
        delete this->_stack;
    }
}

int CPU::switch_context(Context *from, Context *to)
{   
    if (from && to ){
        ucontext_t *currentContextPtr = &from->_context;
        ucontext_t *nextContextPtr = &to->_context;
        int swapWorked = swapcontext(currentContextPtr, nextContextPtr);
        return swapWorked;
    } else {
        return -1;
    }
}

int CPU::finc(volatile int & number)
{
    register int result = 1;
    asm("lock xadd %0, %2" : "=a"(result) : "a"(result), "m"(number));

    return result;
}

int CPU::fdec(volatile int & number)
{
    register int result = -1;
    asm("lock xadd %0, %2" : "=a"(result) : "a"(result), "m"(number));

    return result;
}

__END_API
