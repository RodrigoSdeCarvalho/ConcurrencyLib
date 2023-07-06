#include <iostream>
#include <ucontext.h>
#include <queue>
#include <chrono>
#include <ctime>

#include "Concurrency/thread.h"

__BEGIN_API

using namespace std;

int Thread::_available_id = -1; // Se o valor for -1, significa que ainda não foi criada nenhuma thread.
// Primeira thread criada terá id 0.

int Thread::_numOfThreads = 0;

queue<int> Thread::_released_ids;

Thread *Thread::_running;

Thread Thread::_main;

CPU::Context Thread::_main_context;

Thread Thread::_dispatcher;

Thread::Ready_Queue Thread::_ready;

Thread::Ready_Queue Thread::_suspended;

void Thread::init(void (*main)(void *))
{
    // Cria a thread main, passando main() e a string "Main" como parâmetros.
    // A string é argumento da função main().
    create_main_thread(main);

    // Cria a thread dispatcher, que será responsável por escolher a próxima thread a ser executada.
    create_dispatcher_thread();

    // Pega o contexto da main() do main.cc e salva em _main_context.
    new (&_main_context) CPU::Context();

    // Troca o contexto da main() do main.cc para o contexto da Thread::_main criada aqui.
    CPU::switch_context(&_main_context, _main.context());
}

void Thread::create_main_thread(void (*main)(void *))
{
    new (&_main) Thread(main, (void *)"Main");
    _running = &_main;
    _main._state = RUNNING;
}

void Thread::create_dispatcher_thread()
{
    new (&_dispatcher) Thread(&dispatcher);
}

int Thread::id()
{
    return this->_id;
}

int Thread::get_now_timestamp()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int Thread::get_available_id()
{
    if (Thread::_released_ids.empty())
        Thread::_available_id++;
    else
    {
        Thread::_available_id = Thread::_released_ids.front();
        Thread::_released_ids.pop();
    }

    return Thread::_available_id;
} // retorna o id da thread, que é um atributo privado.

void Thread::dispatcher()
{
    while (!_ready.empty())
    {
        // Escolhe a próxima thread a ser executada.
        // E já a prepara, setando seu estado e o ponteiro _running.
        Thread* nextThreadToRun = get_thread_to_dispatch_ready();

        // Torna o dispatcher a thread que está executando no momento.
        prepare_dispatcher_to_run_again();

        db<Thread>(TRC) << "THREAD" << nextThreadToRun->_id << "EM EXECUÇÂo" << "\n";
        db<Thread>(TRC) << "READY QUEUE: " << _ready.size() << "\n";

        // Troca o contexto para a próxima thread a ser executada.
        // Ou seja, a partir daqui, a próxima thread a ser executada é a que acabou de ser escolhida.
        // Assim, a próxima linha após a troca de contexto só será executada quando a thread escolhida terminar sua execução
        // e o despachante voltar a ser executado.
        Thread::switch_context(&_dispatcher, nextThreadToRun);

        // Ao voltar ao despachante, verifica se a próxima thread a ser executada (que está no começo da fila)
        // terminou sua execução. Se sim, a removerá da fila de prontos.
        check_if_next_thread_is_finished();
    }

    // Caso a fila esteja vazia, acabaram a execução todas as threads.
    // Assim, o despachante retorna para a thread main.
    return_to_main();
}

void Thread::return_to_main()
{
    db<Thread>(TRC) << "THREAD MAIN EM EXECUÇÃO" << "\n";
    _dispatcher._state = FINISHING;
    switch_context(&_dispatcher, &_main);
}

void Thread::yield()
{
    // Imprima informação usando o debug em nível TRC;
    db<Thread>(TRC) << "Yield Chamado"; // Imprime a thread que está executando.

    // Escolha uma próxima thread a ser executada;
    // Como o Dispacher não chama o yield, ele não é rankeado novamente, então sua prioridade sempre será a maior.
    // Logo, next aponta ao Dispacher. E este, portanto, dispara a próxima thread a ser executada.
    Thread * next = _ready.remove()->object();
    Thread * prev = _running;

    // Atualiza a prioridade da tarefa que estava sendo executada (aquela que chamou yield) com o
    // timestamp atual, a fim de reinserí-la na fila de prontos atualizada (cuide de casos especiais, como
    // estado ser FINISHING ou Thread main que não devem ter suas prioridades alteradas);
    if (_running != &_main && _running->_state != FINISHING && _running->_state != SUSPEND && _running->_state != WAITING)
    {
        _running->_link.rank(get_now_timestamp()); // Atualiza a prioridade da thread que estava executando.
        db<Thread>(TRC) << "\nTHREAD " << _running->_id << " RANKEADA COM NOW TIMESTAMP = " << _running->_link.rank() << ".\n";

        // Reinsira a thread que estava executando na fila de prontos;
        _running->_state = READY;
        _ready.insert(&_running->_link);
        db<Thread>(TRC) << "\nTHREAD " << _running->_id << " REINSERIDA NA FILA DE PRONTAS.\n";
    }

    // Atualiza o ponteiro _running;
    _running = next;

    // Atualiza o estado da próxima thread a ser executada;
    next->_state = RUNNING;

    // Troque o contexto entre as threads;
    db<Thread>(TRC) << "\nTHREAD " << prev->_id << " TEVE SEU CONTEXTO TROCADO PARA " << next->_id << ".\n";

    switch_context(prev, next);
}

void Thread::insert_thread_link_on_ready_queue(Thread* thread)
{
    if (thread->_id != _main._id)
    {
        _ready.insert(&thread->_link);
    }
}

int Thread::join()
{
    if (_running == this)
    {
        db<Thread>(TRC) << "Thread::join() CHAMADO PELA PRÓPRIA THREAD.\n";
        return -1;
    }

    if (this->_state != FINISHING)
    {
        _waiting = _running;
        _running->suspend();
    }
    return _exit_code;
}

void Thread::resume()
{
    if (this->_state == SUSPEND)
    {
        _suspended.remove(&_link);
        this->_state = READY;
        _ready.insert(&_link);
    }
}

void Thread::suspend()
{
    // Seta o estado da thread como SUSPEND.
    _state = SUSPEND;

    _suspended.insert(&_link); // Insere a thread na fila de suspensas.

    if (_running != this)
    {
        _ready.remove(&_link); // Remove a thread da fila de prontos.
    }
    else
    {
        yield();
    }
}

void Thread::sleep(Asleep_Queue* sleepQueue)
{
    if (!_asleep)
    {
        _asleep = sleepQueue;
    }

    sleepQueue->insert(&_link);

    db<Thread>(TRC) << "Thread::sleep() CHAMADO.\n";
    _state = WAITING;
    if (_running != this)
    {
        _ready.remove(&_link);
    }
    else
    {
        yield();
    }
}

void Thread::wakeup(bool reschedule)
{
    db<Thread>(TRC) << "Thread::wakeup() CHAMADO.\n";
    _state = READY;
    _link.rank(get_now_timestamp());
    _ready.insert(&_link);

    if (reschedule)
        yield();
}

void Thread::thread_exit(int exit_code)
{
    db<Thread>(INF) << "THREAD " << this->_id << " DELETADA.\n";
    _numOfThreads--; // Decrementa o número de threads criadas.
    _released_ids.push(this->_id); // Coloca o id da thread que está sendo encerrada na fila de ids liberados.
    this->_state = FINISHING; // Seta o estado da thread como finalizando.
    this->_exit_code = exit_code; // Seta o código de término da thread.

    // Se houver uma thread suspensa por estar esperando a execução desta thread terminar, a libera.
    if (_waiting)
    {
        _waiting->resume();
        _waiting = nullptr;
    }

    yield(); // Libera o processador para outra thread(DISPACHER).
}

Thread::~Thread()
{
    if (_asleep)
    {
        _asleep->remove(&_link);
    }

    _ready.remove(&this->_link); // Remove a thread da fila de prontos.
    if (this->_context) // Libera o contexto, caso ele exista.
    {
        delete this->_context;
    }
}

__END_API