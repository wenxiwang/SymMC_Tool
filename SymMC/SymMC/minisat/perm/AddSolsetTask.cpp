#include "AddSolsetTask.h"

AddSolsetTask::AddSolsetTask(size_t hash_value, list<PermThread>& ptList) {
    this->hash_value = hash_value;
    for(auto& pt: ptList) {
        this->subSetList.emplace_back(std::move(pt.isomSolSets));
    }
}
