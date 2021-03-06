#include "pmdk_queue.h"

int pmem_queue::init(pool_base &pop,int cap)
{
    transaction::run(pop, [&]
    {

        if(capacity == 0)
        {
            head = make_persistent<pmem_entry>();
            head->key = nullptr;
            head->value = make_persistent<p_string>(pop,"this is a head");
            head->next = nullptr;
            head->prev = nullptr;
            capacity = cap;
            queue_size = 0;
            tail = head;
        }

    });
    queue_hash = new std::map<std::string,int>();
    std::cout<<(*queue_hash).size()<<std::endl;
    queue_hash->clear();
    std::cout<<"init"<<std::endl;
    persistent_ptr<pmem_entry> temp = head->next;
    while(temp)
    {
        (*queue_hash)[temp->key->data()] = 1;
        temp = temp->next;
    }
    return 1;
}
int pmem_queue::search_node(char* key)
{
    return (*queue_hash).find(key) != (*queue_hash).end();
}
int pmem_queue::update_node(pool_base &pop,char* key)
{
//        transaction::run(pop, [&] {
    persistent_ptr<pmem_entry> temp = head->next;
//    std::cout<<tail->key->compare(key)<<std::endl;
//    std::cout<<tail->key->data()<<std::endl;
//    std::cout<<key<<std::endl;
    if(tail->key->compare(key))
        return 1;
    while(temp)
    {
        if(temp->key->compare(key))
        {
            temp->prev->next = temp->next;
            if(temp->next)
                temp->next->prev = temp->prev;
            break;
        }
        temp = temp->next;
    }
    if(temp == nullptr)
        return 0;
    tail->next = temp;
    temp->next = nullptr;
    temp->prev = tail;
    tail = temp;
    return 1;
//		});
}
int pmem_queue::push(pool_base &pool,char* key,char* value)
{
    transaction::run(pool, [&]
    {
        if(search_node(key))
        {
            update_node(pool,key);
        }
        else
        {
            if(queue_size == capacity)
                delete_persistent<pmem_entry>(pop(pool));
            tail->next = make_persistent<pmem_entry>();
            tail->next->prev = tail;
            tail->next->next = nullptr;
            tail = tail->next;
            tail->key = make_persistent<p_string>(pool,std::string(key));
            tail->value = make_persistent<p_string>(pool,std::string(value));
            (*queue_hash)[key] = 1;
            queue_size = queue_size + 1;
        }
    });
    return 0;
}
persistent_ptr<pmem_entry> pmem_queue::pop(pool_base &pool)
{
    persistent_ptr<pmem_entry> temp = head->next;
    transaction::run(pool, [&]
    {
        (*queue_hash).erase(temp->key->data());
        if(temp != nullptr)
        {
            head->next = temp->next;
            if(head->next)
                head->next->prev = head;
            queue_size = queue_size - 1;
        }
    });
    return temp;
}
persistent_ptr<pmem_entry> pmem_queue::del(char* key)
{
    persistent_ptr<pmem_entry> temp = head->next;
    while(temp)
    {
        if(temp->key->compare(key))
        {
            temp->prev->next = temp->next;
            if(temp->next)
                temp->next->prev = temp->prev;
            break;
        }
        temp = temp->next;
    }
    if(temp == nullptr)
        return 0;
    temp->prev->next = temp->next;
    if(temp == tail)
        tail = temp->prev;
    else
        temp->next->prev = temp->prev;
    (*queue_hash).erase(temp->key->data());
    queue_size = queue_size - 1;
    return temp;
}
void pmem_queue::print(pool_base &pop)
{
    persistent_ptr<pmem_entry> temp = tail;
    while(temp && temp != head)
    {
        std::cout<<temp->key->data()<<" ";
        temp = temp->prev;
    }
    std::cout<<"\n";
}

pmem_queue::~pmem_queue()
{
//    std::cout<<"this is delte pmem_queue"<<std::endl;
    persistent_ptr<pmem_entry> temp = head->next;
    while(temp != nullptr)
    {
        temp = temp->next;
        if(temp != nullptr)
            delete_persistent<pmem_entry>(temp->prev);
    }
    head = nullptr;
    tail = nullptr;
    queue_hash->clear();
}
