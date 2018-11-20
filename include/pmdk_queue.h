#ifndef _PMDK_QUEUE_
#define _PMDK_QUEUE_

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj_cpp_examples_common.hpp>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <map>

#define LAYOUT "queue"

using pmem::obj::delete_persistent;//delete
using pmem::obj::make_persistent;//new
using pmem::obj::p;//Êý¾Ý
using pmem::obj::persistent_ptr;//Ö¸Õë
using pmem::obj::pool;
using pmem::obj::pool_base;
using pmem::obj::transaction;

class p_string
{
public:
    persistent_ptr<char[]> data_;
    p<size_t> size_;

    p_string(pool_base& pop, const std::string& src)
    {
        transaction::run(pop, [&]
        {
            data_ = make_persistent<char[]>(src.size() + 1);
            memcpy(&data_[0], src.c_str(), src.size());
            data_[src.size()] = 0;
            size_ = src.size();
//            std::cout<<&data_[0]<<std::endl;
        });

    }

    int compare(const std::string& right)
    {
        return strcmp(&data_[0], right.c_str());
    }

    char* data()
    {
        return &data_[0];
    }

    size_t size()
    {
        return size_;
    }
};

class pmem_entry
{
public:
    persistent_ptr<pmem_entry> next,prev;
    persistent_ptr<p_string> value;
    persistent_ptr<p_string> key;
};

class pmem_queue
{
public:
    p<int> capacity;
    p<int> queue_size;
    persistent_ptr<pmem_entry> head,tail;
    std::map<char*,int>* queue_hash;

    ~pmem_queue();
    int init(pool_base &pop,int cap);
    int search_node(char* key);
    int update_node(pool_base &pop,char* key);
    int push(pool_base &pool,char* key,char* value);
    int isFull()
    {
        return queue_size == capacity;
    }
    persistent_ptr<pmem_entry> pop(pool_base &pool);
    persistent_ptr<pmem_entry> del(char* key);
    void print(pool_base &pop);
};

#endif // _PMDK_QUEUE_
