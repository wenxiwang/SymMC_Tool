

#include "ReduceThread.h"

void* merge(void* input) {
    auto rt = (ReduceThread*) input;
    rt->a.insert(rt->b.begin(), rt->b.end());
    rt->r = std::move(rt->a);
    return NULL;
}

ReduceThread::ReduceThread() {

}

ReduceThread::ReduceThread(ReduceThread& e) {
    a = e.a;
    b = e.b;
    r = e.r;
    thd = e.thd;
}

ReduceThread::ReduceThread(ReduceThread&& e) noexcept:
        a(std::move(e.a)),
        b(std::move(e.b)),
        r(std::move(e.r)),
        thd(e.thd)
{

}

void ReduceThread::start() {
    pthread_create(&thd, NULL, merge, (void*) this);
}

void ReduceThread::join() {
    pthread_join(thd, NULL);
}
