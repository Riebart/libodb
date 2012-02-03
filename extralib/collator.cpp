#include "collator.hpp"

#include <stdlib.h>
#include <string.h>

DataCollator::~DataCollator()
{
    std::list<struct row*>::iterator it = rows.begin();
    std::list<struct row*>::iterator end = rows.end();
    
    while (it != end)
    {
        free(*it);
        rows.erase(it);
    }
}

DataCollator::DataCollator()
{
    context = NULL;
    handler = NULL;
    fd = NULL;
    start_offset = 0;
    automated_output = false;
}

DataCollator::DataCollator(FILE* _fd)
{
    DataCollator();
    
    this->fd = fd;
    automated_output = true;
}

DataCollator::DataCollator(void (*_handler)(void* context, uint32_t length, void* data), void* _context)
{
    DataCollator();
    
    this->handler = _handler;
    this->context = _context;
    automated_output = true;
}

void DataCollator::add_data(uint32_t offset, uint32_t length, void* data)
{
    struct row* row = (struct row*)malloc(sizeof(struct row) + length - 1);
    
    if (row == NULL)
    {
        throw -1;
    }
    
    row->offset = offset;
    row->length = length;
    memcpy(&(row->data), data, length);
    
    // If we successfully add it to the front, write out a bunch of data if we
    // have an automated system configured.
    if (try_add_front(row))
    {
        struct row* cur = get_data_p();
        
        while (cur != NULL)
        {
            if (fd != NULL)
            {
                fwrite(&(cur->data), cur->length, 1, fd);
            }
            else if (handler != NULL)
            {
                handler(context, cur->length, &(cur->data));
            }
            
            free(cur);
            cur = get_data_p();
        }
    }
    else
    {
        add_back(row);
    }
}

struct DataCollator::row* DataCollator::get_data_p()
{
    struct row* ret = NULL;
    
    if (rows.size() > 0)
    {
        ret = rows.front();
        
        if (ret != NULL)
        {
            rows.erase(rows.begin());
        }
    }
    
    return ret;
}

void* DataCollator::get_data(uint32_t* length)
{
    void* ret = NULL;
    
    // If we're got a NULL head, or either of the automatic output options
    // are configured, return NULL.
    if (!automated_output)
    {
        struct row* r = get_data_p();
        
        if (r != NULL)
        {
            *length = r->length;
            ret = &(r->data);
        }
        else
        {
            ret = NULL;
        }
    }
    
    return ret;
}

struct DataCollator::row* DataCollator::truncate_row_end(struct DataCollator::row* row, uint32_t new_length)
{
    struct row* r2 = (struct row*)malloc(sizeof(struct row) + new_length - 1);
    
    if (r2 == NULL)
    {
        throw -1;
    }
    
    r2->offset = start_offset;
    r2->length = row->length - (start_offset - row->offset);
    memcpy(&(r2->data), &(row->data) + (start_offset - row->offset), r2->length);
    
    return r2;
}

struct DataCollator::row* DataCollator::truncate_row_start(struct DataCollator::row* row, uint32_t new_length)
{
    struct row* r2 = (struct row*)malloc(sizeof(struct row) + new_length - 1);
    
    if (r2 == NULL)
    {
        throw -1;
    }
    
    r2->offset = start_offset;
    r2->length = row->length - (start_offset - row->offset);
    memcpy(&(r2->data), &(row->data), r2->length);
    
    return r2;
}

bool DataCollator::try_add_front(struct row* row)
{
    // The cases here are:
    //  - The list is empty
    //  - The list starts with a NULL sentinel in which case we can try to add it.
    //  - The list starts with a non-NULL
    //   > This only happens if there are no automatic output mechanism set up.
    //     In which case we can just hop out and we'll add the new item from the end.
    
    std::list<struct row*>::iterator it = rows.begin();
    std::list<struct row*>::iterator it2 = rows.end();
    
    bool ret = false;
    
    if (start_offset > row->offset)
    {
        throw -2;
    }
    
    // If the list is empty...
    if (it == it2)
    {
        // If the current offset is larger than the expected start, then we need
        // a NULL sentinel at the start.
        if (start_offset < row->offset)
        {
            rows.push_front(NULL);
        }
        
        rows.push_back(row);
        
        ret = true;
    }
    else if ((*it) == NULL)
    {
        it2 = it;
        ++it;
        
        // Now erase the first sentinel NULL if we don't need it anymore.
        if (start_offset == row->offset)
        {
            rows.erase(it2);
        }
        
        // Insert right before the first non-NULL row.
        rows.insert(it, row);
        
        // If the first non-NULL row starts later then the current one ends,
        // we need to put a sentinel between them.
        if ((*it)->offset > (row->offset + row->length))
        {
            rows.insert(it, NULL);
        }
        
        ret = true;
    }
    
    return ret;
}

void DataCollator::add_back(struct row* row)
{
    std::list<struct row*>::reverse_iterator it = rows.rbegin();
    std::list<struct row*>::reverse_iterator end = rows.rend();
    std::list<struct row*>::reverse_iterator prev = end;
    
    // We can initialize this to whatever we like because we are guaranteed to
    // reach a non-NULL value before we hit a NULL location.
    uint32_t last_offset = (uint32_t)(-1);
    
    while (it != end)
    {
        // We only insert if we've found a possible spot to insert.
        if ((*it) == NULL)
        {
            // If our current end is before the start of the last non-NULL we 
            // enocuntered, and we've got an opening, we insert here.
            if ((row->offset + row->length) <= last_offset)
            {
                std::list<struct row*>::reverse_iterator next = ++it;
                
                // Check for overlap of the next non-NULL.
                // Since we already handled the 'front' insertion case, we don't
                // need to worry about running into a case of next == end;
                if (((*next)->offset + (*next)->length) > row->offset)
                {
                    throw -2;
                }
                // If we start right after the next row, remove the NULL.
                else if (((*next)->offset + (*next)->length) == row->offset)
                {
                    rows.erase(it.base());
                }

                // If we leave space between this and the previous one, insert
                // a NULL.
                if ((row->offset + row->length) < last_offset)
                {
                    rows.insert(prev.base(), NULL);
                }
                
                rows.insert(prev.base(), row);
                break;
            }
        }
        else
        {
            // last_offset holds the *start* of the row we might preced, which 
            // is pointed to by 'prev'
            last_offset = (*it)->offset;
        }
        
        prev = it;
        ++it;
    }
}
