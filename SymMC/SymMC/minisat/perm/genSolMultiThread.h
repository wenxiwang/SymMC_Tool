#ifndef MINISAT_GENSOLMULTITHREAD_H
#define MINISAT_GENSOLMULTITHREAD_H

#include <pthread.h>

static void* run(void* input);
class genSolMultiThread {

    public:
        pthread_t thd;
        genSolMultiThread();
        void start();
        void join();
};


#endif //MINISAT_GENSOLMULTITHREAD_H
