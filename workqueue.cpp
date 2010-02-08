
#include "workqueue.hpp"
#include "workitembase.hpp"
#include <time.h>

void * thread_run(void * queue);

WorkQueue::WorkQueue (int n_num_threads)
{
    num_threads=n_num_threads;
    
   
    pthread_t temp;
    
    int i;
    for (i=0; i<num_threads; i++)
    {
        threads.push_back(temp);
        pthread_create(&threads[i], NULL, &thread_run, &queue);
    }

}


WorkQueue::~WorkQueue()
{
    
}

void WorkQueue::add_work_item(WorkItemBase * newItem, int priority)
{

    queue.push_back(newItem);
    
}


//This is shit, but it is simple
void * thread_run(void * queue)
{
    struct timespec sleep_time;
    
    sleep_time.tv_sec=0;
    sleep_time.tv_nsec=1000;

    WorkItemBase * curr_item;
    while (1)
    {
        while ( ((std::deque<WorkItemBase *> *)queue)->empty() )
        {
//             pthread_yield();
            nanosleep(&sleep_time, NULL);
        }
        
        //TODO:race condition here - lock the queue?
        curr_item=((std::deque<WorkItemBase *> *)queue)->front();
        ((std::deque<WorkItemBase *> *)queue)->pop_front();
        
        curr_item->do_work();
    }
    
    return NULL;
}



