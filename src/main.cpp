#include "pmdk_multiqueue.h"
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>

using namespace std;

class randCreate
{
private:
    static randCreate* randCreater;
    randCreate()
    {
        srand(time(NULL));
    }
public:
    static randCreate* getCreater()
    {
        if(randCreater == NULL)
            randCreater = new randCreate();
        return randCreater;
    }
    int get(int low,int high)
    {
        if(high < low)return -1;
        return rand()%(high - low + 1) + low;
    }
};
randCreate* randCreate::randCreater = NULL;
/*
int main(int argc,char *argv[])
{

//    ofstream file("out.txt");
//    streambuf* strm_buffer = std::cout.rdbuf();
//    std::cout.rdbuf(file.rdbuf());
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
//        input = randCreate::getCreater()->get(0,80);
        std::cout<<"input: "<<input<<std::endl;
        sprintf(itc,"%d",input);
        if(input == -1)
            break;
        if(input == 0)
            r->mq->do_decay(pop);
        else
            r->mq->push(pop,itc,itc);
        r->mq->print();
    }
    r->mq->print();

    while(true)
    {
        std::cin>>input;
        sprintf(itc,"%d",input);
        if(input == -1)
            break;
        persistent_ptr<pmem_entry> entry = r->mq->lookup(pop,itc);
        if(entry == nullptr)
            std::cout<<"not found"<<endl;
        else
            std::cout<<entry->value->data()<<endl;
    }
    return 0;
}
*/


int main(int argc,char *argv[])
{

//    ofstream file("out.txt");
//    streambuf* strm_buffer = std::cout.rdbuf();
//    std::cout.rdbuf(file.rdbuf());
    const char *path = argv[1];
    class rnode
    {
    public:
        persistent_ptr<pmem_queue> mq = nullptr;
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
            r->mq = make_persistent<pmem_queue>();
            init(pop,4);
        else
        {
            ;//r->mq->hash_recovery(pop);
        }
    });
    int i = 100,input = 0;
    char itc[100];
    while(i--)
    {
        std::cin>>input;
//        input = randCreate::getCreater()->get(0,80);
        std::cout<<"input: "<<input<<std::endl;
        sprintf(itc,"%d",input);
        if(input == -1)
            break;
        else
            r->mq->push(pop,itc,itc);
        r->mq->print(pop);
    }
}
