

#ifndef CPP_REDUCETHREAD_H
#define CPP_REDUCETHREAD_H

#include "dep.h"

class ReduceThread {
    public:
    pthread_t thd;

    unordered_set<size_t> a;
    unordered_set<size_t> b;
    unordered_set<size_t> r;

    ReduceThread();
    ReduceThread(ReduceThread&);
    ReduceThread(ReduceThread&&) noexcept;
    void start();
    void join();

};


#endif //CPP_REDUCETHREAD_H
