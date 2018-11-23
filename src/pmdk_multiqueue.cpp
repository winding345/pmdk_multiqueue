#include "pmdk_multiqueue.h"

pmem_multiqueue::pmem_multiqueue(int multi_num,int queue_len,int default_level)
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
    history_map = new std::map<std::string,int>();
    mq_hash = new std::map<std::string,block_info*>();
    std::cout<<history_queue->queue_size<<"size"<<std::endl;
}

int pmem_multiqueue::hash_recovery(pool_base &pop)
{
    std::cout<<"hash_recovery"<<std::endl;
    history_map = new std::map<std::string,int>();
    mq_hash = new std::map<std::string,block_info*>();
    //恢复过程中所有的热度均为1，history_map中的对应的原队列为0队列
    persistent_ptr<pmem_entry> temp;
    //遍历mq
    for(int i = 0;i < multi_num;++i)
    {
        mq[i]->init(pop,queue_len);
        temp = mq[i]->tail;
        while(temp &&temp != mq[i]->head)
        {
            (*mq_hash)[temp->key->data()] = new block_info(READ_VALUE,i);
            temp = temp->prev;
        }
    }
    //遍历history_map
    temp = history_queue->tail;
    history_queue->init(pop,queue_len);
    while(temp &&temp != history_queue->head)
    {
        (*mq_hash)[temp->key->data()] = new block_info(READ_VALUE,-1);
        (*history_map)[temp->key->data()] = 0;
        temp = temp->prev;
    }
    print();
    return 1;
}

int pmem_multiqueue::search_node(char* key)
{
    if((*mq_hash).find(key) == (*mq_hash).end())
        return -2;
    return (*mq_hash)[key]->level;
}

int pmem_multiqueue::push(pool_base &pop,char* key,char* value)
{
    int level = search_node(key);
    if(level != -2)
        return update(pop,key,level);
    persistent_ptr<pmem_queue> op_queue = mq[default_level];
    if(op_queue->isFull())
    {
        mq2history(pop,default_level);
    }

    op_queue->push(pop,key,value);
    (*mq_hash)[key] = new block_info(READ_VALUE,default_level);

    return 1;
}

int pmem_multiqueue::update(pool_base &pop,char* key,int level)
{
    std::cout<<"update"<<std::endl;
    if(level == -1)
        return history2mq(pop,key);
    //热度提升
    (*mq_hash)[key]->hot += READ_VALUE;
    if(level + 1 != multi_num && (*mq_hash)[key]->hot >= HOT_LEVEL)
    {
        return levelup(pop,key,level);
    }
    return mq[level]->update_node(pop,key);
}

persistent_ptr<pmem_entry> pmem_multiqueue::pop(pool_base &pool)
{
    if(history_queue->queue_size == 0)
        return NULL;
    persistent_ptr<pmem_entry> temp = history_queue->pop(pool);
    history_map->erase(temp->key->data());
    delete((*mq_hash)[temp->key->data()]);
    mq_hash->erase(temp->key->data());
    return temp;
}

int pmem_multiqueue::levelup(pool_base &pop,char* key,int level)
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
        mq2history(pop,level+1);
    return op_queue->push(pop,temp->key->data(),temp->value->data());
}

int pmem_multiqueue::mq2history(pool_base &pool,int level)
{
    std::cout<<"mq2history"<<std::endl;
    persistent_ptr<pmem_entry> entry = mq[level]->pop(pool);
    if(history_queue->isFull())
    {
        pop(pool);
    }
    history_queue->push(pool,entry->key->data(),entry->value->data());
    (*mq_hash)[entry->key->data()]->level = -1;
    (*history_map)[entry->key->data()] = level;
    return 1;
}

int pmem_multiqueue::history2mq(pool_base &pop,char* key)
{
    std::cout<<"history2mq"<<std::endl;
    persistent_ptr<pmem_entry> temp = history_queue->del(key);
    if(temp == NULL)
        return -1;
    int level = (*history_map)[temp->key->data()];
    (*history_map).erase(temp->key->data());

    //热度提升
    (*mq_hash)[key]->hot += READ_VALUE;
    if(level + 1 != multi_num &&(*mq_hash)[key]->hot >= HOT_LEVEL)
    {
        (*mq_hash)[key]->hot = READ_VALUE;
        ++level;
    }
    (*mq_hash)[key]->level = level;
    persistent_ptr<pmem_queue> op_queue = mq[level];
    if(op_queue->isFull())
        mq2history(pop,level);
    return op_queue->push(pop,temp->key->data(),temp->value->data());
}

persistent_ptr<pmem_entry> pmem_multiqueue::lookup(pool_base &pool,char* key)
{
    int level = search_node(key);
    if(level == -2)
        return nullptr;
    update(pool,key,level);
    return mq[search_node(key)]->tail;
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
        delete_persistent<pmem_queue>(temp);
        mq[multi_num-1] = make_persistent<pmem_queue>();
        mq[multi_num-1]->init(pop,queue_len);
        std::map<std::string,block_info* >::iterator it;
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
            std::cout<<temp->key->data()<<"("<<(*mq_hash)[temp->key->data()]->hot<<","<<(*mq_hash)[temp->key->data()]->level<<")"<<" ";
            temp = temp->prev;
        }
        std::cout<<std::endl;
    }

    std::cout<<"print history("<<history_queue->queue_size<<")"<<std::endl;
    temp = history_queue->tail;
    while(temp &&temp != history_queue->head)
    {
        std::cout<<temp->key->data()<<"("<<(*mq_hash)[temp->key->data()]->hot<<","<<(*mq_hash)[temp->key->data()]->level<<","<<(*history_map)[temp->key->data()]<<")"<<" ";
        temp = temp->prev;
    }
    std::cout<<"\n\n";
}
