#include "pmdk_multiqueue.h"

pmem_multiqueue::pmem_multiqueue(pool_base &pop,int multi_num,int queue_len,int default_level)
:multi_num(multi_num),queue_len(queue_len),default_level(default_level)
{
    transaction::run(pop, [&]
    {
        mq = make_persistent<persistent_ptr<pmem_queue>[]>(multi_num);
        for(int i = 0;i < multi_num;++i)
        {
            mq[i] = make_persistent<pmem_queue>();
            mq[i]->init(pop,queue_len);
        }
        history_queue = make_persistent<pmem_queue>();
        history_queue->init(pop,queue_len);
    });
    std::cout<<history_queue->queue_size<<"size"<<std::endl;
}

int pmem_multiqueue::search_node(uint64_t key)
{
    if((*mq_hash).find(key) == (*mq_hash).end())
        return -2;
    return (*mq_hash)[key]->level;
}

int pmem_multiqueue::push(pool_base &pop,uint64_t key,char* value)
{
    int level = search_node(key);
    if(level != -2)
        return update(key,level);
    persistent_ptr<pmem_queue> op_queue = mq[default_level];
    if(op_queue->isFull())
        mq2history(default_level);
    op_queue->push(pop,key,value);
    (*mq_hash)[key] = new block_info(READ_VALUE,default_level);
    return 1;
}

int pmem_multiqueue::update(pool_base &pop,uint64_t key,int level)
{
    std::cout<<"update"<<std::endl;
    if(level == -1)
        return history2mq(key);
    //热度提升
    (*mq_hash)[key]->hot += READ_VALUE;
    if(level + 1 != multi_num && (*mq_hash)[key]->hot >= HOT_LEVEL)
    {
        return levelup(key,level);
    }
    return mq[level]->update_node(key);
}

persistent_ptr<pmem_entry> pmem_multiqueue::pop()
{
    if(history_queue->queue_size == 0)
        return NULL;
    persistent_ptr<pmem_entry> temp = history_queue->pop();
    history_map->erase(temp->key);
    delete((*mq_hash)[temp->key]);
    mq_hash->erase(temp->key);
    return temp;
}

int pmem_multiqueue::levelup(pool_base &pop,uint64_t key,int level)
{
    std::cout<<"levelup"<<std::endl;
    if(level >= multi_num - 1)
        return 0;
    persistent_ptr<pmem_queue> op_queue = mq[level];
    persistent_ptr<pmem_entry> temp = op_queue->del(key);
    if(temp == NULL)
        return -1;
    op_queue = mq[level + 1];
    (*mq_hash)[key]->level = level + 1;
    (*mq_hash)[key]->hot = READ_VALUE;
    if(op_queue->isFull())
        mq2history(level+1);
    return op_queue->push(temp->key,temp->value);
}

int pmem_multiqueue::mq2history(pool_base &pop,int level)
{
    std::cout<<"mq2history"<<std::endl;
    persistent_ptr<pmem_entry> entry = mq[level]->pop();
    if(history_queue->isFull())
        pop();
    history_queue->push(entry->key,entry->value);
    (*mq_hash)[entry->key]->level = -1;
    (*history_map)[entry->key] = level;
    return 1;
}

int pmem_multiqueue::history2mq(pool_base &pop,uint64_t key)
{
    std::cout<<"history2mq"<<std::endl;
    persistent_ptr<pmem_entry> temp = history_queue->del(key);
    if(temp == NULL)
        return -1;
    int level = (*history_map)[temp->key];
    (*history_map).erase(temp->key);

    //热度提升
    (*mq_hash)[key]->hot += READ_VALUE;
    if(level + 1 != multi_num &&(*mq_hash)[key]->hot >= HOT_LEVEL)
    {
        (*mq_hash)[key]->hot = READ_VALUE;
        ++level;
    }
    (*mq_hash)[key]->level = level;
    std::cout<<temp->key<<std::endl;
    persistent_ptr<pmem_queue> op_queue = mq[level];
    if(op_queue->isFull())
        mq2history(level);
    std::cout<<temp->key<<std::endl;
    return op_queue->push((uint64_t)temp->key,(char*)temp->value);
}

int pmem_multiqueue::do_decay(pool_base &pop)
{
    transaction::run(pop, [&]
    {
        persistent_ptr<pmem_queue> temp = mq[0];
        for(int i = 0;i < multi_num - 1;++i)
        {
            mq[i] = mq[i+1];
        }
        delete(temp);
        mq[multi_num-1] = make_persistent<pmem_queue>();
        mq[multi_num-1]->init(pop,queue_len);
        std::map<uint64_t,block_info* >::iterator it;
        for(it = mq_hash->begin();it!=mq_hash->end();)
        {
            if(it->second->level < 0)
            {
                it++;
                continue;
            }
            it->second->level -= 1;
            if(it->second->level < 0)
            {
                delete(it->second);
                mq_hash->erase(it++);
            }
            else
                it++;
        }
    });
    return 1;
}

void pmem_multiqueue::print()
{
    std::cout<<"print multi-queue"<<std::endl;
    persistent_ptr<pmem_entry> temp;
    for(int i = 0;i < multi_num;++i)
    {
        std::cout<<"<"<<i<<"("<<mq[i]->queue_size<<")>"<<'\t';
        temp = mq[i]->tail;
        while(temp &&temp != mq[i]->head)
        {
            std::cout<<temp->key<<"("<<(*mq_hash)[temp->key]->hot<<","<<(*mq_hash)[temp->key]->level<<")"<<" ";
            temp = temp->prev;
        }
        std::cout<<std::endl;
    }

    std::cout<<"print history("<<history_queue->queue_size<<")"<<std::endl;
    temp = history_queue->tail;
    while(temp &&temp != history_queue->head)
    {
        std::cout<<temp->key<<"("<<(*mq_hash)[temp->key]->hot<<","<<(*mq_hash)[temp->key]->level<<","<<(*history_map)[temp->key]<<")"<<" ";
        temp = temp->prev;
    }
    std::cout<<"\n\n";
}
