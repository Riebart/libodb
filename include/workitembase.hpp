#ifndef WORK_ITEM_BASE_HPP
#define WORK_ITEM_BASE_HPP

class WorkItemBase
{
public:
    WorkItemBase(): priority(0) { };
    virtual void do_work()=0;
    inline void set_priority(int n_priority) { priority=n_priority; };
    inline int get_priority() { return priority; };

private:
    int priority;
};



#endif
