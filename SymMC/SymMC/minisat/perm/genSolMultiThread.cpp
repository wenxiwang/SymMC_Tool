
#include "genSolMultiThread.h"
#include <pthread.h>
#include "MCSampler.h"

genSolMultiThread::genSolMultiThread() {

}

void genSolMultiThread::start() {
    int a = pthread_create(&thd, NULL, run, (void*) this);
    if(a!=0) {
        cout << "PermThread created failed!! a=" << a << endl;
    }
}

void genSolMultiThread::join() {
    pthread_join(thd, NULL);
}

void* run(void* input) {
    while(true) {
        pthread_mutex_lock(&lock4batchvec);
        if(!batchvec.empty()) {
            string line = *batchvec.rbegin();
            batchvec.pop_back();
            pthread_mutex_unlock(&lock4batchvec);

            isNonIsom = true;
            std::hash<string> str_hash;
            size_t hash_value = str_hash(line);

            GenPermSolMultiThd(line, hash_value);
            if (isNonIsom) {
                nonIsomSolSets.insert(hash_value);
            }
            AlloySolNum++;
        }
        else if(!minisatRun){
            pthread_mutex_unlock(&lock4batchvec);
            break;
        }
        else {
            pthread_mutex_unlock(&lock4batchvec);
        }
    }
    noMoreTask = true;
    return NULL;
}
