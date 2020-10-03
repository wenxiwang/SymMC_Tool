

#ifndef CPP_MCSAMPLER_H
#define CPP_MCSAMPLER_H

#include "dep.h"
#include "type.h"
#include "PermPair.h"
#include "PermThread.h"
#include "BasicPermPair.h"
#include "ReduceThread.h"
#include "AddSolsetTask.h"
#include "AddSolsetThread.h"
class AddSolsetTask;
class AddSolsetThread;

extern size_t AlloySolNum;
extern size_t threadNum;
extern size_t timeout; //(in seconds)


extern size_t MIN_COMB_NUM;
extern size_t MAX_COMB_NUM;
extern double EXP_COMB_SR;

extern mpf_class real_comb_sr;

extern size_t min_perm_num;
extern size_t max_perm_num;
extern double exp_perm_sr;

extern unordered_set<size_t> nonIsomSolSets;
extern mpf_class permNum;

extern size_t cntFSB;

extern bool is_sampling;
extern bool isNonIsom;
extern mpf_class ratioNSB;
extern mpf_class ratioFSB;
extern mpf_class isomSolCnt;
extern pthread_mutex_t lock4isomCnt;
extern pthread_mutex_t lock4taskQ;
extern pthread_mutex_t lock4batchvec;
extern bool noMoreTask;
extern queue <AddSolsetTask> taskQueue;
extern int batch;
extern vector<string> batchvec;
extern vector<PermPair> ppList;
extern bool minisatRun;
//extern set<list<list<int>>> cycleList;
mpf_class factorial(size_t);
extern size_t asstThreadNum;
extern list<AddSolsetThread> asstList;
int parseBasicPerms(string&, list<vector<int>>&, list<BasicPermPair>&);
list<list<list<list<int>>>> getComboCycleList(list<vector<int>>&, L4&);
list<list<list<list<int>>>> GenAllCombo(L4&);
L3 GenAllPerms(vector<int>&, bool);
list<list<list<list<int>>>> SampPerms(list<vector<int>>&, mpf_class);
unordered_set<size_t> sampling(size_t, size_t);
list<list<int>> genc4Oneperm(vector<int>&, vector<int>&);
L2P getTransPosList(list<list<list<list<int>>>>&);
void genPermList(L2P&, list<BasicPermPair>&);
void fillPermPair(int, int, list<BasicPermPair>&, PermPair&);
void genSolSets(string&);
void genSolSetsMultiThd(string&);
void genSolSetsOneThd(string&);

void GenPermSolMultiThd(string& , size_t);
void GenPermSolSingThd(string& , size_t);

void checkSolSets();
bool clearAsstList();

void genPermList();


void numlist_shuffle(list<vector<int>>&);
size_t numlist_hash(list<vector<int>>&);


list<string> split(string c, string k);
#endif //CPP_MCSAMPLER_H
