#ifndef _PMDK_MULTI_QUEUE_
#define _PMDK_MULTI_QUEUE_

#include "pmdk_queue.h"
#include "define.h"

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

    std::map<uint64_t,int>* history_map;//记录history中数据对应的原来queue
    std::map<uint64_t,block_info* >* mq_hash;

    pmem_multiqueue(pool_base &pop,int multi_num,int queue_len,int default_level);
    int hash_recovery();
    int search_node(uint64_t key);
    int push(pool_base &pop,uint64_t key,char* value);
    int update(pool_base &pop,uint64_t key,int level);
    persistent_ptr<pmem_entry> pop(pool_base &pool);
    int levelup(pool_base &pop,uint64_t key,int level);
//    int leveldown(uint64_t key,int level);
    int mq2history(pool_base &pop,int level);
    int history2mq(pool_base &pop,uint64_t key);
//
    int do_decay(pool_base &pop);
    void print();
};

#endif // _PMDK_MULTI_QUEUE_

