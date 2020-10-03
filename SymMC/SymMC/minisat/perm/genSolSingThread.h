

#ifndef MINISAT_GENSOLSINGTHREAD_H
#define MINISAT_GENSOLSINGTHREAD_H


#include <pthread.h>

static void* run(void* input);
class genSolSingThread {

public:
    pthread_t thd;
    genSolSingThread();
    void start();
    void join();
};


#endif //MINISAT_GENSOLSINGTHREAD_H
