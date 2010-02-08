#ifndef WORK_QUEUE_HPP
#define WORK_QUEUE_HPP

#include <pthread.h>
#include <stdint.h>
#include <deque>
#include <vector>

class WorkItemBase;

class WorkQueue
{

public:
    WorkQueue(int n_num_threads);
    ~WorkQueue();
    void add_work_item(WorkItemBase * new_Item, int priority=0);
    inline uint32_t get_num_items() { return queue.size(); };
    
private:
    int num_threads;
    std::deque<WorkItemBase *> queue;
    std::vector<pthread_t> threads;

};





#endif
