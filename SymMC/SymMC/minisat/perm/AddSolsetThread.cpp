//thread for calculating each isomCnt for each solution;

//#include <zconf.h>
#include "AddSolsetThread.h"
//#include <unistd.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


/*AddSolsetThread::AddSolsetThread(size_t hash_value, list<PermThread>& ptList) {
    this->oneSolset.insert(hash_value);
    this->isomCnt = 0;
    this->finished = false;

    for(auto& pt: ptList) {
        this->subSetList.emplace_back(std::move(pt.isomSolSets));
    }
}*/

AddSolsetThread::AddSolsetThread() {
    //this->oneSolset.insert(hash_value);
   // this->isomCnt = 0;
   // this->finished = false;
}

void AddSolsetThread::start() {
    int a = pthread_create(&thd, NULL, run, (void*) this);
    if(a!=0) {
        cout << "addSolsetThread created failed!! a=" << a << endl;
    }
}

void AddSolsetThread::join() {
    pthread_join(thd, NULL);
}

void* run(void* input) {
    unordered_set<size_t> oneSolset;
        while(true) {
                pthread_mutex_lock(&lock4taskQ);
                if(!taskQueue.empty()) {
                    //cout << "taskqueue not empty!!" << endl;
                    AddSolsetTask t = taskQueue.front();
                    taskQueue.pop();
                    pthread_mutex_unlock(&lock4taskQ);

                    oneSolset.insert(t.hash_value);
                    for (auto & ss : t.subSetList) {
                        oneSolset.insert(ss.begin(), ss.end());
                        //ss.clear();
                    }

                    pthread_mutex_lock(&lock4isomCnt);
                    isomSolCnt += oneSolset.size();
                   // cout << "isomSolCnt: " << isomSolCnt << endl;
                    pthread_mutex_unlock(&lock4isomCnt);

                    oneSolset.clear();
                    t.subSetList.clear();
                }
                else if(noMoreTask){
                    //cout << "noMoreTask" << endl;
                    pthread_mutex_unlock(&lock4taskQ);
                    break;
                }
                else {
                    //cout << "none" << endl;
                    //sleep(onesolTime.count());
                    pthread_mutex_unlock(&lock4taskQ);
                }
        }

    return NULL;
}
