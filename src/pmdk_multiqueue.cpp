#include "pmdk_multiqueue.h"

pmem_multiqueue::pmem_multiqueue(pool_base &pop,int multi_num,int queue_len,int default_level)
{
//    for(int i = 0;i < multi_num;++i)
//    {
//        pmem_queue* q = new pmem_queue(queue_len);
//        mq.push_back(q);
//        hot_range.push_back(HOT_LEVEL);
//    }
//    history_queue = new pmem_queue(queue_len);
    if(this->default_level != 0)
    {
        //
        return;
    }
    this->multi_num = multi_num;
    this->queue_len = queue_len;
    this->default_level = default_level;
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
    std::cout<<history_queue->capacity<<std::endl;
}

