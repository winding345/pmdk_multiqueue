#include "pmdk_multiqueue.h"

int main(int argc,char *argv[])
{
    const char *path = argv[1];
    class rnode
    {
    public:
        persistent_ptr<pmem_multiqueue> mq = nullptr;
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
        if(r->mq == nullptr)
            r->mq = make_persistent<pmem_multiqueue>(pop,4,2,1);
        else
        {
            r->mq->hash_recovery(pop);
        }
    });
    int i = 100,input = 0;
    char itc[100];
    while(i--)
    {
        std::cin>>input;
        sprintf(itc,"%d.bmp",i);
        std::cout<<input<<" "<<itc<<std::endl;
        if(input == -1)
            break;
        if(input == -2)
            r->mq->do_decay(pop);
        else
            r->mq->push(pop,input,itc);
        r->mq->print();
    }
    return 0;
}
