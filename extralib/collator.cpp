#include "collator.hpp"

#include <stdlib.h>
#include <string.h>

DataCollator::~DataCollator()
{
    std::list<struct row*>::iterator it = rows.begin();
    std::list<struct row*>::iterator end = rows.end();
    
    if (fd != NULL)
    {
        fflush(fd);
    }
    else if (handler != NULL)
    {
        handler(context, 0, NULL);
    }

    while (it != end)
    {
        if ((*it) != NULL)
        {
            free(*it);
        }
        
        it = rows.erase(it);
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

    this->fd = _fd;
    automated_output = true;
}

DataCollator::DataCollator(void (*_handler)(void* context, uint32_t length, void* data), void* _context)
{
    DataCollator();

    this->handler = _handler;
    this->context = _context;
    automated_output = true;
}

void DataCollator::add_data(int32_t offset, uint32_t length, void* data)
{
    // Just bail if we're not actually adding anything.
    if (length == 0)
    {
        return;
    }
    
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
        if (automated_output)
        {
            struct row* cur = get_data_p();

            while (cur != NULL)
            {
                if (fd != NULL)
                {
                    fwrite(&(cur->data), cur->length, 1, fd);
                }
                if (handler != NULL)
                {
                    handler(context, cur->length, &(cur->data));
                }

                free(cur);
                cur = get_data_p();
            }
        }
    }
    else
    {
        add_back(row);
    }
}

bool DataCollator::check_overlap(int32_t a, uint32_t aL, int32_t b, uint32_t bL)
{
    // First, ensure that A starts before B
    if (a > b)
    {
        int32_t t1 = a;
        a = b;
        b = t1;
        
        uint32_t t2 = aL;
        aL = bL;
        bL = t2;
    }
    
    return ((a+aL) > b);
}

struct DataCollator::row* DataCollator::get_data_p()
{
    struct row* ret = NULL;

    if (rows.size() > 0)
    {
        ret = rows.front();

        if (ret != NULL)
        {
            rows.pop_front();
            start_offset += ret->length;
        }
    }

    return ret;
}

struct DataCollator::row* DataCollator::get_data()
{
    struct row* ret = NULL;

    // If we're got a NULL head, or either of the automatic output options
    // are configured, return NULL.
    if (!automated_output)
    {
        ret = get_data_p();
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

bool DataCollator::try_add_front(struct DataCollator::row* row)
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
        
        // Now check to see if we actually are inserting here. This happens if
        // thew new row starts before the first non-NULL row. Also check for
        // overlap.
        if ((*it)->offset > row->offset)
        {
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
        // Check for overlap.
        else if (check_overlap((*it)->offset, (*it)->length, row->offset, row->length))
        {
            throw -2;
        }
    }

    return ret;
}

void DataCollator::add_back(struct DataCollator::row* row)
{
    std::list<struct row*>::reverse_iterator it = rows.rbegin();
    std::list<struct row*>::reverse_iterator end = rows.rend();
    std::list<struct row*>::reverse_iterator prev = end;

    // We can initialize this to whatever we like because we are guaranteed to
    // reach a non-NULL value before we hit a NULL location.
    int32_t last_offset = 0;

    while (it != end)
    {
        // We only insert if we've found a possible spot to insert.
        if ((*it) == NULL)
        {
            // Move ahead so that prev and it surround a NULL. We're going to see
            // if we can insert at the bordered NULL.
            ++it;
            
            // If we start at or after the lower row, and end at or before the
            // upper one, we can insert.
            if (((row->offset + row->length) <= (*prev)->offset) &&
                 (row->offset >= ((*it)->offset + (*it)->length)))
            {
                // 'close' as in 'close call' not as in 'close the door'
                bool close_start = (row->offset == ((*it)->offset + (*it)->length));
                bool close_end = ((row->offset + row->length) == (*prev)->offset);
                
                if (close_start)
                {
                    // Insert immedaitely after the lower bound.
                    rows.insert(it.base(), row);
                    
                    if (close_end)
                    {
                        rows.erase(it.base());
                    }
                }
                else if (close_end)
                {
                    rows.insert(prev.base(), row);
                }
                else
                {
                    std::list<struct row*>::iterator i = it.base();
                    i = rows.insert(i, row);
                    rows.insert(i, NULL);
                }

                break;
            }
            // Check to see if we the previous interval.
            else if (check_overlap(row->offset, row->length, (*it)->offset, (*it)->length))
            {
                throw -2;
            }
            
        }
        else
        {
            // Handle the case of where we're at the end of the list separately.
            if (prev == end)
            {
                // The only time we'll insert here is if we come after the end
                int32_t cur_offset = (*it)->offset + (*it)->length;
                
                if (cur_offset <= row->offset)
                {
                    if (cur_offset < row->offset)
                    {
                        rows.push_back(NULL);
                    }
                    
                    rows.push_back(row);
                    break;
                }
                // Check for overlap.
                else if (check_overlap((*it)->offset, (*it)->length, row->offset, row->length))
                {
                    throw -2;
                }
            }
            
            // last_offset holds the *start* of the row we might preced, which
            // is pointed to by 'prev'
            last_offset = (*it)->offset;
        }

        prev = it;
        ++it;
    }
}
