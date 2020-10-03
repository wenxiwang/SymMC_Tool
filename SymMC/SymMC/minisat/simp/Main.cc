/*****************************************************************************************[Main.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007,      Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <errno.h>
#include <zlib.h>
#include <signal.h>
#include "minisat/utils/System.h"
#include "minisat/utils/ParseUtils.h"
#include "minisat/utils/Options.h"
#include "minisat/core/Dimacs.h"
#include "minisat/simp/SimpSolver.h"
#include <bits/stdc++.h> 
#include <iostream> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <pthread.h>
#include <iomanip>
#include <unistd.h>
#include <sys/resource.h>

#include "minisat/perm/MCSampler.h"
#include "minisat/perm/genSolMultiThread.h"
#include "minisat/perm/genSolSingThread.h"

using namespace Minisat;

//=================================================================================================


static Solver* solver;
// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
static void SIGINT_interrupt(int) { solver->interrupt(); }
int primary_var;

// Note that '_exit()' rather than 'exit()' has to be used. The reason is that 'exit()' calls
// destructors and may cause deadlocks if a malloc/free function happens to be running (these
// functions are guarded by locks for multithreaded use).
static void SIGINT_exit(int) {
    printf("\n"); printf("*** INTERRUPTED ***\n");
    if (solver->verbosity > 0){
        solver->printStats();
        printf("\n"); printf("*** INTERRUPTED ***\n"); }
    _exit(1); }

void printStats(Solver& solver)
{
    double cpu_time = cpuTime();
    double mem_used = memUsedPeak();
    printf("restarts              : %"PRIu64"\n", solver.starts);
    printf("conflicts             : %-12"PRIu64"   (%.0f /sec)\n", solver.conflicts   , solver.conflicts   /cpu_time);
    printf("decisions             : %-12"PRIu64"   (%4.2f %% random) (%.0f /sec)\n", solver.decisions, (float)solver.rnd_decisions*100 / (float)solver.decisions, solver.decisions   /cpu_time);
    printf("propagations          : %-12"PRIu64"   (%.0f /sec)\n", solver.propagations, solver.propagations/cpu_time);
    printf("conflict literals     : %-12"PRIu64"   (%4.2f %% deleted)\n", solver.tot_literals, (solver.max_literals - solver.tot_literals)*100 / (double)solver.max_literals);
    if (mem_used != 0) printf("Memory used           : %.2f MB\n", mem_used);
    printf("CPU time              : %g s\n", cpu_time);
}


//=================================================================================================

void genSolsMulti() {
    minisatRun = true;
    vec<Lit> blocking_clause;

    if (pthread_mutex_init(&lock4isomCnt, NULL) != 0)
        printf("\n lock4isomCnt mutex init failed\n");

    if (pthread_mutex_init(&lock4taskQ, NULL) != 0)
        printf("\n lock4taskQ mutex init failed\n");

    if (pthread_mutex_init(&lock4batchvec, NULL) != 0)
        printf("\n lock4batchvec mutex init failed\n");

    genSolMultiThread gensolT = genSolMultiThread();
    gensolT.start();
    for(int i=0; i< asstThreadNum; i++) {
        AddSolsetThread asst = AddSolsetThread();
        asstList.emplace_back(asst);
        asstList.back().start();
    }

    while(solver->solve()) {
        blocking_clause.clear();
        std::string line = "";
        for (int i = 0; i < primary_var; i++) {
            blocking_clause.push(mkLit(i, solver->modelValue(i) == l_True));
            if(solver->modelValue(i) == l_True) {
                line += "1";
            }
            else {
                line += "0";
            }
        }

        pthread_mutex_lock(&lock4batchvec);
        batchvec.emplace_back(line);
        pthread_mutex_unlock(&lock4batchvec);

        solver->addClause_(blocking_clause);
    }
    minisatRun = false;

    for(AddSolsetThread &asst: asstList) {
        asst.join();
    }
    gensolT.join();

   // cout << "end of gensols" << endl;
    pthread_mutex_destroy(&lock4isomCnt);
    pthread_mutex_destroy(&lock4taskQ);
    pthread_mutex_destroy(&lock4batchvec);
}

void genSolsSingle() {
    //cout << "gensolssignle" << endl;
    if (pthread_mutex_init(&lock4batchvec, NULL) != 0)
        printf("\n lock4batchvec mutex init failed\n");

    minisatRun = true;

    genSolSingThread gensolT = genSolSingThread();
    gensolT.start();

    vec<Lit> blocking_clause;
    while(solver->solve()) {
        blocking_clause.clear();
        std::string line = "";
        for (int i = 0; i < primary_var; i++) {
            blocking_clause.push(mkLit(i, solver->modelValue(i) == l_True));
            if(solver->modelValue(i) == l_True) {
                line += "1";
            }
            else {
                line += "0";
            }
        }

        pthread_mutex_lock(&lock4batchvec);
        batchvec.emplace_back(line);
        pthread_mutex_unlock(&lock4batchvec);

        solver->addClause_(blocking_clause);
    }
    minisatRun = false;

    gensolT.join();
    pthread_mutex_destroy(&lock4batchvec);
}

void genSols(int onePermMEffort ) {

    int permSize = ppList.size();
    size_t processor_count = std::thread::hardware_concurrency();
    
    if(permSize < 50 && onePermMEffort < 1500) {
	genSolsSingle();	
	return;
    } 

    if(onePermMEffort < 1500) {
	threadNum = 2;
	asstThreadNum = processor_count-threadNum;
    }
    else if(permSize >= 50000){
        threadNum = 2;
	threadNum = std::min(threadNum + (onePermMEffort-1500)/1500, processor_count/2); 
    	asstThreadNum = processor_count-threadNum;  
    }
    else {
    	threadNum = 2;
	threadNum = std::min(threadNum + (onePermMEffort-1500)/1500, processor_count-2); 
    	asstThreadNum = processor_count-threadNum;  
    } 

    genSolsMulti();
}


void* main_thread_run(void* input) {
    char** argv = (char**) input;
    string permF(argv[2]);

    list<vector<int>> numList;

    list<BasicPermPair> basicPPList;
    int onePermMEffort = parseBasicPerms(permF, numList, basicPPList);

    L4 l4perms;
    list<list<list<list<int>>>> comboCycleList =  getComboCycleList(numList, l4perms);
  

    numList.clear();

    L2P transposList = getTransPosList(comboCycleList);
    comboCycleList.clear();
    l4perms.clear();

    vector<PermPair> ppList(transposList.size());
    genPermList(transposList, basicPPList);
    genSols(onePermMEffort);
    return NULL;
}

int main(int argc, char** argv)
{    
    try {	
        setUsageHelp("USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or gzipped DIMACS.\n");
        
#if defined(__linux__) && defined(_FPU_EXTENDED) && defined(_FPU_DOUBLE) && defined(_FPU_GETCW)
        fpu_control_t oldcw, newcw;
        _FPU_GETCW(oldcw); newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE; _FPU_SETCW(newcw);
        //printf("WARNING: for repeatability, setting FPU to use double precision\n");
#endif
        // Extra options:
        //
        IntOption    verb   ("MAIN", "verb",   "Verbosity level (0=silent, 1=some, 2=more).", 0, IntRange(0, 2));
        IntOption    cpu_lim("MAIN", "cpu-lim","Limit on CPU time allowed in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX));
        IntOption    mem_lim("MAIN", "mem-lim","Limit on memory usage in megabytes.\n", INT32_MAX, IntRange(0, INT32_MAX));
        BoolOption   cnt_all("MAIN", "all",    "Count all models.", false);
        BoolOption   use_pol("MAIN", "pol",    "Set preferred polarity for minimal model search.", true);
	
        parseOptions(argc, argv, true);

        Solver S;
        auto startTime = chrono::high_resolution_clock::now();
        //double initial_time = cpuTime();

        solver = &S;
        // Use signal handlers that forcibly quit:
        signal(SIGINT, SIGINT_exit);
        signal(SIGXCPU,SIGINT_exit);

        if (argc == 1)
            printf("Reading from standard input... Use '--help' for help.\n");

       // printf("argv[1]: %s\n", argv[1]);
        
        gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
        if (in == NULL)
            printf("ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]), exit(1);
        
        if (verb > 0){
            printf("============================[ Problem Statistics ]=============================\n");
            printf("|                                                                             |\n"); }
        
        primary_var = parse_DIMACS(in, S);

        //cout << "primary_var: " << primary_var << endl;
        gzclose(in);
        
        if (verb > 0){
            printf("|  Number of variables:  %12d                                         |\n", S.nVars());
            printf("|  Number of clauses:    %12d                                         |\n", S.nClauses()); }

    	struct timespec stoptime;
    	if (clock_gettime(CLOCK_REALTIME, &stoptime) == -1) {
        	cerr << "Failed to get curr time" << endl;
        	return 0;
    	}
   	    stoptime.tv_sec += timeout;

    	pthread_t mainthd;
    	pthread_create(&mainthd, NULL, main_thread_run, (void *) argv);

    	int err = 0;
    	err = pthread_timedjoin_np(mainthd, NULL, &stoptime);

    	bool is_time_out = false;
    	if(err == ETIMEDOUT) {
        	is_time_out = true;
    	}

    	checkSolSets();

        auto endTime = chrono::high_resolution_clock::now();
    	chrono::duration<double, std::milli> time_span = endTime - startTime;	

    	if(is_time_out) {
		cout << "TimeOut!" << endl;
        	try {
            	pthread_kill(mainthd, SIGKILL);
        	}
        	catch(exception e) {
        	}
    	}

	cout << "\n Cnt_NSB: " << isomSolCnt << "\n C_FSB: " << cntFSB << "\n Metric: " << ratioFSB << "\n PermNum: "
    	<< permNum << "\n Time: " <<time_span.count()/1000.0 << setprecision(4) << endl;
        
    } catch (OutOfMemoryException&){
        printf("===============================================================================\n");
        printf("INDETERMINATE\n");
        exit(0);
    }
}
