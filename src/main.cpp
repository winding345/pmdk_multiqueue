#include "pmdk_multiqueue.h"

int main(int argc,char *argv[])
{
    const char *path = argv[1];
    class rnode
    {
    public:
        persistent_ptr<pmem_multiqueue> mq;
    };

    pool<rnode> pop;
    if (file_exists(path) != 0)
    {
        pop = pool<rnode>::create(path, LAYOUT, PMEMOBJ_MIN_POOL, CREATE_MODE_RW);
    }
    else
    {
        pop = pool<rnode>::open(path, LAYOUT);
    }
    auto r = pop.root();
    transaction::run(pop, [&]
    {
         r->mq = make_persistent<pmem_multiqueue>(pop,1,2,3);
    });
}
