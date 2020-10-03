#ifndef CPP_ADDSOLSETTHREAD_H
#define CPP_ADDSOLSETTHREAD_H

#include <pthread.h>
#include <thread>
#include "MCSampler.h"
//#include "AddSolsetTask.h"

static void* run(void* input);

class AddSolsetThread {
public:
   // unordered_set<size_t> oneSolset;
   // int isomCnt;
   // list<unordered_set<size_t>> subSetList;
    pthread_t thd;
    //AddSolsetThread(size_t hash_value, list<PermThread>& ptList);
    AddSolsetThread();
    void start();
    void join();
};


#endif //CPP_ADDSOLSETTHREAD_H
