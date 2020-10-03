

#ifndef CPP_PERMTHREAD_H
#define CPP_PERMTHREAD_H

#include <assert.h>
#include <pthread.h>
#include <thread>
#include "PermPair.h"
#include "MCSampler.h"

static void* run(void* input);

class PermThread {
    public:
    string org;
    //non isomorphic solution set;
    int beginIndx;
    int endIndx;


    pthread_t thd;
    unordered_set<size_t> isomSolSets;

    PermThread(string org, int beginIndx, int endIndx);
    void clearSubSolSets();
    void start();
    void join();
};


#endif //CPP_PERMTHREAD_H
