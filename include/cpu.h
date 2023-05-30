#ifndef cpu_h
#define cpu_h

#include <ucontext.h>
#include <iostream>
#include "traits.h"

__BEGIN_API

class CPU
{
    public:

        class Context
        {
        private:
            static const unsigned int STACK_SIZE = Traits<CPU>::STACK_SIZE;
        public:
            Context() { _stack = 0; };

            template<typename ... Tn>
            Context(void (* func)(Tn ...), Tn ... an) {
                allocateStack(); // aloca espaço para a pilha do contexto.
                save(); // inicializa o contexto em _context. Que será usado no makecontext.

                this->_context.uc_link = 0; // ponteiro ao contexto que seria carregado após o retorno do contexto atual. 
                                            // porém, como não haverá tal retorno, esse valor é 0.
                setContextStack(); 

                ucontext_t *newContextPtr =  &this->_context;
                makecontext(newContextPtr, (void(*)())(func), sizeof...(Tn), an...); // Cria o novo contexto.
            }

            ~Context();

            void save();
            void load();

        private:            
            char *_stack;

            void allocateStack() {
                    this->_stack = new char[STACK_SIZE];
                }

            void setContextStack() {
                this->_context.uc_stack.ss_flags = 0;
                this->_context.uc_stack.ss_sp = this->_stack; // seta o stack pointer do contexto para a pilha alocada.
                this->_context.uc_stack.ss_size = STACK_SIZE; // seta o tamanho da pilha do contexto.
            }

        public:
            ucontext_t _context;
        };

    public:
        static int finc(volatile int & number);
        static int fdec(volatile int & number);
        static int switch_context(Context *from, Context *to);

};

__END_API

#endif
