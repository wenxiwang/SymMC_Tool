#ifndef CPP_ADDSOLSETTASK_H
#define CPP_ADDSOLSETTASK_H
#include "dep.h"
// #include "MCSampler.h"
#include "PermThread.h"
class PermThread;

class AddSolsetTask {
public:
    list<unordered_set<size_t>> subSetList;
    size_t hash_value;
    AddSolsetTask(size_t hash_value, list<PermThread>& ptList);
};


#endif //CPP_ADDSOLSETTASK_H
