

#include "MCSampler.h"

#include <sys/resource.h>


size_t AlloySolNum = 0;
size_t threadNum = 0;
size_t timeout = 5000; //(in seconds)
bool is_sampling = false;
bool hasIdentical = false;
bool isNonIsom = true;
bool minisatRun;
//1000	5000	10000  50000             
//5000	10000	50000  100000
//0.3     0.4     0.5    0.6
size_t MIN_COMB_NUM = 50000; // user set
size_t MAX_COMB_NUM = 100000; // user set
double EXP_COMB_SR = 0.6; // user set

mpf_class real_comb_sr;

size_t cntFSB = 0;

// non-isomorphic solution set;
unordered_set<size_t> nonIsomSolSets;

// isomorphic solution count;
mpf_class isomSolCnt = 0;
mpf_class permNum = 0;
// list of thread for getting each isomCnt for each solution;
list<AddSolsetThread> asstList;
mpf_class ratioNSB;
mpf_class ratioFSB;
size_t asstThreadNum = 0;
pthread_mutex_t lock4isomCnt;
pthread_mutex_t lock4taskQ;
pthread_mutex_t lock4batchvec;
queue <AddSolsetTask> taskQueue;
bool noMoreTask = false;
int batch;
vector<string> batchvec;
vector<PermPair> ppList;


void checkSolSets() {

    if(AlloySolNum == 0) {
        ratioFSB = 0;
        ratioNSB = 0;
        return;
    }

    cntFSB = nonIsomSolSets.size();
    //cout << "cntFSB: "<< cntFSB << endl;
    mpf_class asn = AlloySolNum;
    ratioFSB = (double) cntFSB / asn;
    ratioNSB= isomSolCnt / asn;
    //return  isomSolCnt / asn;
}

void genSolSets(string& solF) {
    if(ppList.size() < 50) {
        threadNum = 1;
        genSolSetsOneThd(solF);
    }
    else {
        genSolSetsMultiThd(solF);
    }
}

void genSolSetsMultiThd(string& solF) {
    ifstream solFile(solF);
    string line;

    if (pthread_mutex_init(&lock4isomCnt, NULL) != 0)
    {
        printf("\n lock4isomCnt mutex init failed\n");
    }

    if (pthread_mutex_init(&lock4taskQ, NULL) != 0)
    {
        printf("\n lock4taskQ mutex init failed\n");
    }

    for(int i=0; i< asstThreadNum; i++) {
            AddSolsetThread asst = AddSolsetThread();
            asstList.emplace_back(asst);
            asstList.back().start();
    }

    while (getline(solFile, line)) {
        //cout << "oneline" << endl;
        isNonIsom = true;
        hash<string> str_hash;
        size_t hash_value = str_hash(line);

        GenPermSolMultiThd(line, hash_value);
        if(isNonIsom) {
            nonIsomSolSets.insert(hash_value);
        }
        AlloySolNum++;
    }

    noMoreTask = true;

    for(AddSolsetThread &asst: asstList) {
        asst.join();
    }

    pthread_mutex_destroy(&lock4isomCnt);
    pthread_mutex_destroy(&lock4taskQ);
}

void genSolSetsOneThd(string& solF) {
    ifstream solFile(solF);
    string line;

    while (getline(solFile, line)) {
        isNonIsom = true;
        hash<string> str_hash;
        size_t hash_value = str_hash(line);

        GenPermSolSingThd(line, hash_value);
        if(isNonIsom) {
            nonIsomSolSets.insert(hash_value);
        }
        AlloySolNum++;
    }
}

void GenPermSolMultiThd(string& org, size_t hash_value) {

        int beginIndex = 0;
        int eachSize = ppList.size()/threadNum + 1;
        list<PermThread> ptList;
        for (int i = 0; i < threadNum; i++) {
            int endIndex = beginIndex + eachSize;
            if(endIndex > ppList.size()) {
                endIndex = ppList.size();
            }

            PermThread pt(org, beginIndex, endIndex);
            ptList.emplace_back(std::move(pt));
            beginIndex = endIndex;
        }
        //cout << "ptlist size in GenPermSol: " << ptList.size() << endl;
        for(auto & pt : ptList) {
            pt.start();
        }
        for (auto & pt : ptList) {
            pt.join();
        }
        if(isNonIsom) {
            //cout << "generating tasks" << endl;
            AddSolsetTask t = AddSolsetTask(hash_value, ptList);
            taskQueue.push(t);
            //cout << "generating tasks: " << taskQueue.size() << endl;
        }
}

void GenPermSolSingThd(string& org, size_t hash_value) {
    unordered_set<size_t> oneSolset;
    oneSolset.insert(hash_value);

    for (int i = 0; i < ppList.size(); i++) {
        auto orgSol = org;
        PermPair& pp = ppList[i];
        bool changed = false;
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
        }

        if (changed) {
            hash<string> str_hash;
            size_t hash_value = str_hash(orgSol);

            if(nonIsomSolSets.find(hash_value) != nonIsomSolSets.end()) {
                isNonIsom = false;
                oneSolset.clear();
                return;
            }
            oneSolset.insert(hash_value);
        }
    }
    isomSolCnt+= oneSolset.size();
}

void genPermList(L2P& transposList, list<BasicPermPair>& basicPPList) {

    int i = 0;
    for (auto & tpairList : transposList) {
        PermPair perm;

        for (auto & pair : tpairList) {
            fillPermPair(pair.first, pair.second, basicPPList, perm);
        }

        ppList.emplace_back(std::move(perm));
        i++;
    }
    transposList.clear();

}

void fillPermPair(int prev, int cur, list<BasicPermPair>& basicPPList, PermPair& perm) {

    for (auto & bpp : basicPPList) {
        list<int>& hp = bpp.getHeadPair();

        int prevIndex = 0;
        for (auto & tmp : hp) {
            if (prev == tmp) {
                break;
            }
            prevIndex++;
        }

        if(prevIndex >= hp.size()) {
            prevIndex = -1;
        }

        int curIndex = 0;
        for (auto & tmp : hp) {
            if (cur == tmp) {
                break;
            }
            curIndex++;
        }

        if(curIndex >= hp.size()) {
            curIndex = -1;
        }

        if (prevIndex != -1 && curIndex != -1) {
            perm.addFirst(&bpp.first);
            perm.addSecond(&bpp.second);
            break;
        }
    }
}

L2P getTransPosList(list<list<list<list<int>>>>& comboCycleList) {
    L2P transposList;
    for (auto & cl : comboCycleList) {
        //every p is the cycle notation of only one permutation of one number group
        LP tpairList;
        for (auto & p : cl) {
            // every pp is each cycle;
            for (auto & pp : p) {

                auto i = pp.rbegin();
                auto lastElm = *i;
                i++;

                for (; i != pp.rend(); i++) {
                    pair<int, int> tpair;

                    tpair.first = lastElm;

                    tpair.second = std::move(*i);
                    tpairList.push_back(std::move(tpair));
                }
            }
        }

        transposList.push_back(std::move(tpairList));
    }

    return std::move(transposList);
}

list<list<list<list<int>>>> getComboCycleList(list<vector<int>>& numList, L4& l4perms) {

    // compute total num. of permutation
    list<list<list<list<int>>>> ccl;
    mpf_class total_perm_num = 1;

    for (auto & nums: numList) {
        total_perm_num *= factorial(nums.size());
    }

    // generate all perms for each symmetry
    for (auto & nums: numList) {
        L3 perms4nums = GenAllPerms(nums, false); 
        l4perms.push_back(std::move(perms4nums));
    }
    ccl = GenAllCombo(l4perms);

    mpf_class ccl_size = ccl.size();
    permNum = ccl_size;
    real_comb_sr = ccl_size / total_perm_num;
    return std::move(ccl);
}

unordered_map<size_t, mpf_class> fmap;
mpf_class factorial(size_t m) {
    if (fmap.find(m) == fmap.end()) {
        mpf_class f = 1;
        for(size_t i = 1; i <= m; i++) {
            mpf_class i_big = i;
            f *= i_big;
        }
        fmap[m] = f;
        return f;
    }

    return fmap[m];
}

list<list<int>> genc4Oneperm(vector<int>& r, vector<int>& num) {
    list<list<int>> c4Oneperm;
    list<int> visited;

    for (auto i = num.begin(), ri = r.begin(); i != num.end(); i++, ri++) {
        list<int> cc;
        int select = *i;

        bool contains = false;
        for (auto &tmp : visited) {
            if (tmp == select) {
                contains = true;
                break;
            }
        }

        if (!contains) {
            int corespnd = *ri; //r.get(i);
            cc.emplace_back(select);
            visited.emplace_back(select);
            while (corespnd != select) {
                cc.emplace_back(corespnd);
                visited.emplace_back(corespnd);

                for (auto tmp1 = r.begin(), tmp2 = num.begin(); tmp2 != num.end(); tmp1++, tmp2++) {
                    if (*tmp2 == corespnd) {
                        corespnd = *tmp1;
                        break;
                    }
                }
            }
            if (cc.size() > 1)
                c4Oneperm.push_back(std::move(cc));
        }
    }

    return std::move(c4Oneperm);
}

L3 GenAllPerms(vector<int>& num, bool do_sampling) {
    //cout << "GenAllPerms" << endl;
    hasIdentical = true;
    L3 cycles;
    mpf_class ori_perm_num = factorial(num.size());

        vector<int> r = num;
        do {
            list<list<int>> c4Oneperm = genc4Oneperm(r, num);
            cycles.push_back(std::move(c4Oneperm));

        } while (std::next_permutation(r.begin(), r.end()));
    return std::move(cycles);
}

inline void numlist_shuffle(list<vector<int>>& numList) {
    for(auto & num: numList) {
        random_shuffle(num.begin(), num.end());
    }
}

inline size_t numlist_hash(list<vector<int>>& numList) {
    hash<string> str_hash;

    vector<int> f;
    for(auto & num : numList) {
        for(auto & ele : num)
            f.push_back(ele);
    }

    std::stringstream r_ss;
    std::copy(f.begin(), f.end(), std::ostream_iterator<int>(r_ss, " "));

    return str_hash(r_ss.str());
}

list<list<list<list<int>>>> GenAllCombo(L4& l4perms) {
    list<list<list<list<int>>>> combinations;
    auto element0 = l4perms.begin();

    for (auto & i: *element0) {
        list<list<list<int>>> newList;
        newList.emplace_back(i);
        combinations.push_back(std::move(newList));
    }

    auto nextList = ++l4perms.begin();
    for (; nextList != l4perms.end(); nextList++) {
        list<list<list<list<int>>>> newCombinations;

        for (list<list<list<int>>>& first : combinations) {
            //for(list<list<list<int>>> first: combinations) {
            for (auto & second : *nextList) {
                //for(list<list<int>> second: nextList) {
                list<list<list<int>>> newList;

                // combine
                /*for (auto & tmp : first) {
                    newList.emplace_back(tmp);
                }*/
                newList = first;

                newList.emplace_back(second);
                newCombinations.push_back(std::move(newList));
            }
            first.clear();
        }
        combinations = std::move(newCombinations);
       // nextList->clear();

    }

    return std::move(combinations);
}


int parseBasicPerms(string& permF, list<vector<int>>& numList, list<BasicPermPair>& basicPPList) {
    ifstream permFile(permF);
    string line;
    int onePermMEffort = 0;
    while (getline(permFile, line)) {
        //cout << line << endl;
        if (!line.empty() && line[0] == '*') {

            istringstream iss (line.substr(1));

            vector<int> nums;
            while (iss.rdbuf()->in_avail() > 0) {
                int ipt = 1;
                char comma = -1;
                iss >> ipt >> comma;
                nums.emplace_back(ipt);
            }
            numList.push_back(std::move(nums));
        } else {
            istringstream iss (line);
            BasicPermPair bpp;
            while(iss.rdbuf()->in_avail() > 0) {
                int first = -1;
                char comma = -1;
                int second = -1;

                iss >> first >> comma >> second;
                if(bpp.headPair.empty()) {
                    bpp.headPair.emplace_back(first);
                    bpp.headPair.emplace_back(second);
                }
                else {
                    if(comma == ',') {
                        bpp.first.emplace_back(first);
                        bpp.second.emplace_back(second);
                    }
                }
            }
	    onePermMEffort += bpp.first.size();
            basicPPList.push_back(std::move(bpp));
        }
    }

    onePermMEffort = onePermMEffort/basicPPList.size();
    return onePermMEffort;
}

