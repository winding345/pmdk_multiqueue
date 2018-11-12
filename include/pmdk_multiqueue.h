#ifndef _PMDK_MULTI_QUEUE_
#define _PMDK_MULTI_QUEUE_

#include "pmdk_queue.h"

class block_info
{
public:
    int hot;
    int level;
    block_info(){}
    block_info(int hot,int level):hot(hot),level(level){}
};

class pmem_multiqueue
{
public:
    persistent_ptr<persistent_ptr<pmem_queue>[]> mq;
    persistent_ptr<pmem_queue> history_queue;

    p<int> multi_num,queue_len,default_level;

    pmem_multiqueue(pool_base &pop,int multi_num,int queue_len,int default_level);
//    int push(uint64_t key,char* value);
//    void print();
};

#endif // _PMDK_MULTI_QUEUE_


//persistent_ptr<int[]> new_array =make_persistent<int[]>(size);
