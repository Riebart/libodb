#include "iterator.hpp"
#include "index.hpp"
#include "datastore.hpp"
#include "common.hpp"

Iterator::Iterator()
{
}

Iterator::Iterator(int ident, uint32_t true_datalen, bool time_stamp, bool query_count)
{
    dataobj = new DataObj(ident);
    this->time_stamp = time_stamp;
    this->query_count = query_count;
    this->true_datalen = true_datalen;
    it = NULL;
}

Iterator::~Iterator()
{
    delete dataobj;
//     NOT_IMPLEMENTED("~Iterator()");
}

DataObj* Iterator::next()
{
    NOT_IMPLEMENTED("next()");
    return NULL;
}

DataObj* Iterator::prev()
{
    NOT_IMPLEMENTED("prev()");
    return NULL;
}

DataObj* Iterator::data()
{
    if (dataobj->data == NULL)
    {
        return NULL;
    }
    else
    {
        return dataobj;
    }
}

void* Iterator::get_data()
{
    return dataobj->data;
}

time_t Iterator::get_time_stamp()
{
//     if (time_stamp)
//         return GET_TIME_STAMP(dataobj->data);
//     else
//         return 0;
    
    return time_stamp ? GET_TIME_STAMP(dataobj->data) : 0;
}

uint32_t Iterator::get_query_count()
{
//     if (query_count)
//     {
//         return GET_QUERY_COUNT(dataobj->data);
//     }
//     else
//     {
//         return 0;
//     }
//     
    return query_count ? GET_QUERY_COUNT(dataobj->data) : 0;
}

void Iterator::update_query_count()
{
    if (query_count)
    {
        UPDATE_QUERY_COUNT(dataobj->data);
    }
}

