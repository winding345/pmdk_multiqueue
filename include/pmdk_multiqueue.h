#ifndef _PMDK_MULTI_QUEUE_
#define _PMDK_MULTI_QUEUE_

#include "pmdk_queue.h"
#include "define.h"

class block_info
{
public:
    int hot;
    int level;
    block_info() {}
    block_info(int hot,int level):hot(hot),level(level) {}
};

class pmem_multiqueue
{
public:
    persistent_ptr<persistent_ptr<pmem_queue>[]> mq;
    persistent_ptr<pmem_queue> history_queue;

    p<int> multi_num,queue_len,default_level;

    std::map<std::string,int>* history_map;//记录history中数据对应的原来queue
    std::map<std::string,block_info* >* mq_hash;

    pmem_multiqueue(pool_base &pop,int multi_num,int queue_len,int default_level);
    int hash_recovery(pool_base &pop);
    int search_node(char* key);
    int push(pool_base &pop,char* key,char* value);
    int update(pool_base &pop,char* key,int level);
    persistent_ptr<pmem_entry> pop(pool_base &pool);
    int levelup(pool_base &pop,char* key,int level);
    int mq2history(pool_base &pop,int level);
    int history2mq(pool_base &pop,char* key);
    persistent_ptr<pmem_entry> lookup(pool_base &pool,char* key);
    int do_decay(pool_base &pop);
    void print();
};

class rnode
{
public:
    persistent_ptr<pmem_multiqueue> mq = nullptr;
};

class MQ_Cache
{
public:
    pool<rnode> aep_pool;
    MQ_Cache(char* path,size_t size,int multi_num,int queue_len,int default_level);
    persistent_ptr<rnode> root_node;
};

#endif // _PMDK_MULTI_QUEUE_

