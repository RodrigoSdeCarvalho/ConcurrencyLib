#include "Concurrency/cpu.h"
#include "Concurrency/debug.h"
#include "Concurrency/traits.h"

#include "Concurrency/semaphore.h"

__BEGIN_API

using namespace std;

void Semaphore::p()
{
    // Este método deve implementar a operacao p (ou sleep) de um semaforo. Deve-se decrementar o
    // inteiro do semaforo de forma atomica (utilizando fdec descrita abaixo) e colocar a Thread para
    // dormir caso a mesma nao conseguir acessar o semaforo (ja existir em uso por outra Thread).

    db<Semaphore>(TRC) << "Semaphore::p called." << "\n";
    // PRECISA GARANTIR ATOMICIDADE.
    if(fdec(_value) < 1)
    {
        sleep();
    }

}

void Semaphore::v()
{
    // Este metodo deve implementar a operacao v (ou wakeup) de um semaforo. Deve-se
    // incrementar o inteiro do semaforo de forma atomica (utilizando finc descrita abaixo) e acordar
    // uma Thread que estiver dormindo no semaforo.

    db<Semaphore>(TRC) << "Semaphore::v called" << "\n";
    // PRECISA GARANTIR ATOMICIDADE.
    if(finc(_value) < 0)
    {
        wakeup();
    }

}

int Semaphore::finc(volatile int & number)
{
    // O metodo finc() deve incrementar o valor do semaforo de forma atomica.
    // Por ser dependente do processador, finc() deve ser implementada na classe CPU.
    return CPU::finc(number);
}

int Semaphore::fdec(volatile int & number)
{
    // O metodo fdec() deve decrementar o valor do semaforo de forma atomica.
    // Por ser dependente do processador, fdec() deve ser implementada na classe CPU.
    return CPU::fdec(number);
}

void Semaphore::sleep()
{
    // O metodo sleep() deve colocar a Thread que nao conseguir acessar o semaforo para dormir e
    // mudar seu estado para WAITING (note que WAITING eh diferente de SUSPENDED do trabalho anterior).
    // A Thread deve ser colocada na fila de dormindo do semaforo.
    db<Semaphore>(TRC) << "Semaphore::sleep called to Thread "<< Thread::_running->id() << "\n";
    _asleep.insert(&Thread::_running->_link);
    Thread::_running->sleep(&_asleep);
}

void Semaphore::wakeup(bool reschedule)
{

    if (!_asleep.empty())
    {
        Thread* thread_to_wakeup = _asleep.remove()->object();
        thread_to_wakeup->wakeup(reschedule);
    }
}

void Semaphore::wakeup_all()
{
    // O metodo wakeup_all() deve acordar todas as Thread que estavam dormindo no semaforo.
    db<Semaphore>(TRC) << "Semaphore::wakeup_all called" << "\n";
    while (!_asleep.empty())
    {
        wakeup();
    }
}

Semaphore::~Semaphore()
{
    wakeup_all();
}

__END_API