#include <iostream>
#include <ucontext.h>
#include <stdio.h>

#include "system.h"
#include "thread.h"

__BEGIN_API

void System::init(void (*main)(void *)) 
{
    db<System>(INF) << "SYSTEM INICIADO.\n";
    setvbuf(stdout, 0, _IONBF, 0); //setvbuf(FILE *stream, char *buf, int type, size_t size);

    Thread::init(main);
}

__END_API