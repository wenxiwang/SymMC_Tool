

#include "PermThread.h"

#include <utility>


PermThread::PermThread(string org,
         int beginIndx, int endIndx) {
    this->org = std::move(org);
    this->beginIndx = beginIndx;
    this->endIndx = endIndx;

}

void PermThread::start() {
    int a = pthread_create(&thd, NULL, run, (void*) this);
    if(a!=0) {
        cout << "PermThread created failed!! a=" << a << endl;
    }
}

void PermThread::join() {
    pthread_join(thd, NULL);
}

void* run(void* input) {
    PermThread* thd = (PermThread*) input;

        for (int i = thd->beginIndx; i < thd->endIndx; i++) {
            PermPair& pp = ppList[i];

            auto orgSol = thd->org;

            bool changed = false;
            //System.out.println("org: " + orgSol.toString());
            auto & first = pp.first;
            auto & second = pp.second;

            assert(first.size() == second.size());
            for (auto pair1 = first.begin(), pair2 = second.begin(); pair1 != first.end(); pair1++, pair2++) {
                assert((*pair1)->size() == (*pair2)->size());
                for (auto idx1 = (*pair1)->begin(), idx2 = (*pair2)->begin(); idx1 != (*pair1)->end(); idx1++, idx2++) {
                    int indx1 = *idx1 - 1;
                    int indx2 = *idx2 - 1;

                    char c1 = orgSol[indx1];
                    char c2 = orgSol[indx2];
                    //if c1!=c2, then swap the two bits
                    if (c1 != c2) {
                        changed = true;
                        //cout << "changed" << endl;
                        orgSol[indx2] = c1;
                        orgSol[indx1] = c2;
                    }
                }
                //System.out.println(orgSol.toString());
            }

            if(!isNonIsom) {
                thd->isomSolSets.clear();
                return NULL;
            }

            //System.out.println("pem: " +orgSol);
            //System.out.println(orgSol.toString());
            //solHashCodeSets.add(orgSol.hashCode());
            //solArrayIntSets.add(string2Array(orgSol.toString()));
            if (changed) {
                hash<string> str_hash;
                size_t hash_value = str_hash(orgSol);

                if(nonIsomSolSets.find(hash_value) != nonIsomSolSets.end()) {
                    //kill the thread and other thread;
                    isNonIsom = false;
                    thd->isomSolSets.clear();
                    return NULL;
                }
                thd->isomSolSets.insert(hash_value);
            }
        }

    return NULL;
}
