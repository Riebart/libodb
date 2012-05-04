/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2012 Michael Himbeault and Travis Friesen
 *
 */

#ifndef COLLATOR_HPP
#define COLLATOR_HPP

#include <stdint.h>
#include <stdio.h>
#include <list>

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

/// Implementation of a data-collation method that ensures data is always obtained sequentially
///
/// It uses a linked list to keep track of data pieces and ensures that they are always output
/// sequentially, regardless of the order in which they are input.
///
/// It is able to determine the order of data by ensuring that each piece of data that is added
/// is accompanied by an offset and a length. Overlapping data raises an exception (-2) when it
/// is noticed.
class DataCollator
{
public:
    /// Struct that defines what a row of data looks like.
#pragma pack(1)
    struct row
    {
        /// Offset into the stream at which this data blocb begins
        int64_t offset;
        /// Number of bytes in this data row
        uint64_t length;
        /// Head of the memory allocated for the data row.
        char data;
    };
#pragma pack()

    /// Destructor that ensures that the list and all rows are cleaned up.
    /// Since we can assume that none of this data is in use outside of this object, we can just
    /// throw away the allocations and free up the memory.
    ~DataCollator();

    /// Standard constructor for initializing members.
    ///
    /// Useful at the application level when the output of the stream is being retrieved 'manually'
    /// and no automatic ouptut mechanism (as provided by the other constructors) is required or
    /// desired.
    DataCollator();

    /// Constructor that instructs the collator to write output to the specified file descriptor.
    /// @param [in] _fd The file descriptor to write data to.
    /// @param [in] _cleanup Whether or not to close the file used for output
    /// upon destruction. Default is false.
    ///
    /// This provides flexibility in allowing the output to be written to either a file, a socket,
    /// or stdin of a process started using popen and acquiring the write (stdin) descriptor.
    DataCollator(FILE* _fd, bool _cleanup = false);

    /// Constructor that instructs the collator to write output to the specified handler function.
    /// @param [in] _handler The function that takes in a data row (length and data pointer) and
    /// does something with it.
    /// @param [in,out] _context An opaque binary row that is passed to the handler function
    /// when it is called allowing handler functions to behave differently based on the collator
    /// that is calling them.
    /// @param [in] _cleanup Whether or not to free the contextual information
    /// upon destruction. Default is false.
    ///
    /// Useful for when more flexibility is needed, but automatic output is still required or
    /// desired.
    DataCollator(void (*_handler)(void* context, uint64_t length, void* data), void* _context, bool _cleanup = false);

    /// This funcdtion makes use of a copy-on-add policy so that data can be added form volatile
    /// locations without additional cost at the application level.
    /// @param [in] offset The offset in the stream that this collator represents at which this
    /// data begins.
    /// @param [in] length The number of bytes in this row of data.
    /// @param [in] data The data row itself.
    ///
    /// This function takes in some data row, a length, and an offset and slots it into the stream
    /// where it is supposed to be. If there are gaps or reorderings, space is left for the missing
    /// elements.
    ///
    /// Throws a "-1" if it is unable to allocate memory for some reason.
    void add_data(int64_t offset, uint64_t length, void* data);

    /// This function truncates all data after a certain offset.
    /// @param [in] offset The offset after which to throw away data.
    /// @return The number of bytes thrown away.
    ///
    /// Used for TCP reassembly where if we get an ack, we need to throw away
    /// everything that we have that came after that ack.
    uint64_t truncate_after(int64_t offset);

    /// Used to manually retrieve any data ready for output.
    /// @return NULL if either of the automatic output methods are configured, otherwise it returns
    /// the contents of the node at the head of the list of rows. It may return NULL indicating that
    /// no data is ready for output. This function does not free the row, leaving it up to the user
    /// to free the memory.
    struct row* get_data();

    /// An alternative way of getting data that ensures that data after a certain
    /// point isn't pulled from the stream. Useful for pulling, for example, from
    /// a TCP stream up to the latest known ACKed byte.
    /// @return NULL if either of the automatic output methods are configured, otherwise it returns
    /// the contents of the node at the head of the list of rows. It may return NULL indicating that
    /// no data is ready for output. This function does not free the row, leaving it up to the user
    /// to free the memory.
    /// @param [in] offset Any bytes strictly after the specified offset will not be returned. This
    /// means that any rows that straddle the specified boundary will not be returned at all, instead
    /// of being returned in part.
    struct row* get_data(int64_t offset);

    /// Get the number of rows, not necessarily consecutive, in the list.
    /// @return The total number of rows currently tracked.
    uint64_t size();

    /// Get the start_offset in the stream.
    /// @return The value of start_offset, which represents how far into the stream
    /// our 'base' is. If we're being diligent about output, this represents something
    /// important, otherwise it is close to meaningless depending on the context.
    int64_t get_start();

private:
    /// Initialize the member variables.
    void init();

    /// Whether or not fclose() is called on the file pointer if it is non-NULL upon
    /// destruction, or whether free() is called on the contextual information
    /// if the handler output is enabled.
    bool cleanup;

    /// If not NULL, ready data gets written out to this file descriptor.
    FILE* fd;

    /// If not NULL, ready data gets passed to this handler function.
    /// The context argument to the handler function allows for differentiation between
    /// different collators.
    void (*handler)(void* context, uint64_t length, void* data);

    /// Contextual information provided to allow state-based handling of the output
    /// of different collators when using handler functions.
    void* context;

    /// Holds a quick way to determine whether or not automated output is configured.
    bool automated_output;

    /// Used to keep track of where in the stream the next expected data should start.
    int64_t start_offset;
    
#warning "TODO: Implement the cursor in the data collator."
    /// Used to keep track of the first byte that is unaccounted for.
    int64_t cursor;

    /// Holds the number of rows inserted into the list so that we aren't calling the
    /// O(n) .size() method of the STL list.
    uint64_t count;

    /// Keeps track of the list of rows allowing for space to exist between rows, representing missing data.
    std::list<struct row*> rows;

    /// Attempts to add the new row to the front of the list, to fill in the first gap.
    /// @param [in] row The row to add.
    bool try_add_back(struct row* row);

    /// Attempts to add the new row to the front of the list, to fill in the first gap.
    /// @return Whether or not it was successful in adding the item to the front
    /// is encoded into the return value. If a NULL is returned, it was added, and
    /// otherwise the value of the row (as it may have been realloc-ed) is sent back.
    /// @param [in] row The row to add.
    struct row* try_add_front(struct row* row);

    /// A function that grabs the contents of the head of the list and pops it off
    /// if it isn't NULL.
    /// @return NULL if either of the automatic output methods are configured, otherwise it returns
    /// the contents of the node at the head of the list of rows. It may return NULL indicating that
    /// no data is ready for output. This function does not free the row, leaving it up to the user
    /// to free the memory.
    struct row* get_data_p();

    /// An alternative way of getting data that ensures that data after a certain
    /// point isn't pulled from the stream. Useful for pulling, for example, from
    /// a TCP stream up to the latest known ACKed byte. If a row is returned, it
    /// is popped off of the list.
    /// @return NULL if either of the automatic output methods are configured, otherwise it returns
    /// the contents of the node at the head of the list of rows. It may return NULL indicating that
    /// no data is ready for output. This function does not free the row, leaving it up to the user
    /// to free the memory.
    /// @param [in] offset Any bytes strictly after the specified offset will not be returned. This
    /// means that any rows that straddle the specified boundary will not be returned at all, instead
    /// of being returned in part.
    struct row* get_data_p(int64_t offset);
};

#include <stdlib.h>
#include <string.h>

DataCollator::~DataCollator()
{
    if (fd != NULL)
    {
        fflush(fd);

        if (cleanup)
        {
            fclose(fd);
        }
    }
    else if (handler != NULL)
    {
        handler(context, 0, NULL);

        if (cleanup && (context != NULL))
        {
            free(context);
        }
    }

//     uint64_t next_offset = start_offset;

    struct row* row;
    while (count > 0)
    {
        row = rows.front();
        
//         if (next_offset < row->offset)
//         {
//             fprintf(stderr, "- ");
//         }
        
//         fprintf(stderr, "%llu ", sizeof(struct row) + row->length - 1);
//         next_offset = row->offset + row->length;

        free(row);
        rows.pop_front();
        count--;
    }

//     if (next_offset != start_offset)
//     {
//         fprintf(stderr, "\n");
//     }
}

void DataCollator::init()
{
    cleanup = false;
    context = NULL;
    handler = NULL;
    fd = NULL;
    start_offset = 0;
    automated_output = false;
    count = 0;
}

DataCollator::DataCollator()
{
    init();
}

DataCollator::DataCollator(FILE* _fd, bool _cleanup)
{
    init();

    this->fd = _fd;
    cleanup = _cleanup;
    automated_output = true;
}

DataCollator::DataCollator(void (*_handler)(void* context, uint64_t length, void* data), void* _context, bool _cleanup)
{
    init();

    this->handler = _handler;
    this->context = _context;
    cleanup = _cleanup;
    automated_output = true;
}

void DataCollator::add_data(int64_t offset, uint64_t length, void* data)
{
    // Just bail if we're not actually adding anything.
    if (length == 0)
    {
        return;
    }

    struct row* row;

    if (start_offset > offset)
    {
        if (start_offset > (int64_t)((offset + length)))
        {
            // If we're here, then there is nothing to add, so just throw away
            // the row and return
            return;
        }

        row = (struct row*)malloc(sizeof(struct row) - 1 + length - (start_offset - offset));

        if (row == NULL)
        {
            throw -1;
        }

        char* cdata = (char*)data;
        row->offset = start_offset;
        row->length = length - (start_offset - offset);
        memcpy(&(row->data), cdata + (start_offset - offset), row->length);
    }
    else
    {
        row = (struct row*)malloc(sizeof(struct row) - 1 + length);

        if (row == NULL)
        {
            throw -1;
        }

        row->offset = offset;
        row->length = length;
        memcpy(&(row->data), data, length);
    }

    // If we successfully add it to the front, write out a bunch of data if we
    // have an automated system configured.
    row = try_add_front(row);
    if (row == NULL)
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
        if (!try_add_back(row))
        {
            throw -100;
        }
    }
}

struct DataCollator::row* DataCollator::get_data_p()
{
    struct row* ret = NULL;

    if (count > 0)
    {
        ret = rows.front();

        if (ret->offset == start_offset)
        {
            rows.pop_front();
            count--;
            start_offset += ret->length;
        }
    }

    return ret;
}


struct DataCollator::row* DataCollator::get_data_p(int64_t offset)
{
    struct row* ret = NULL;

    if (count > 0)
    {
        ret = rows.front();

        if (((ret->offset + ret->length) <= offset) && (ret->offset == start_offset))
        {
            rows.pop_front();
            count--;
            start_offset += ret->length;
        }
        else
        {
            ret = NULL;
        }
    }

    return ret;
}

struct DataCollator::row* DataCollator::get_data()
{
    return (automated_output ? NULL : get_data_p());
}

struct DataCollator::row* DataCollator::get_data(int64_t offset)
{
    return (automated_output ? NULL : get_data_p(offset));
}

struct DataCollator::row* DataCollator::try_add_front(struct DataCollator::row* row)
{
    // The cases here are:
    //  - The list is empty
    //  - The list starts with a NULL sentinel in which case we can try to add it.
    //  - The list starts with a non-NULL
    //   > This only happens if there are no automatic output mechanism set up.
    //     In which case we can just hop out and we'll add the new item from the end.

    struct row* ret = row;

    if (count == 0)
    {
        rows.push_front(row);
        count++;
        ret = NULL;
    }
    else
    {
        struct row* f = rows.front();

        // Now check to see if we actually are inserting here. This happens if
        // thew new row starts before the first non-NULL row. Also check for
        // overlap.
        if (f->offset > row->offset)
        {
            // If we hang out past the end of the open first block then we need to add in
            // the first block, memmove the back end of the new row to the front, and mark
            // it for insertion at the back end.
            if ((row->offset + row->length) > f->offset)
            {
                // Create a new row and add it at the front.
                // This row has the length that is the difference between
                struct row* r2 = (struct row*)malloc(sizeof(struct row) - 1 + f->offset - row->offset);

                if (r2 == NULL)
                {
                    throw -1;
                }

                r2->length = f->offset - row->offset;
                r2->offset = row->offset;
                memcpy(&(r2->data), &(row->data), r2->length);
                rows.push_front(r2);
                count++;

                // Move the data on the old row, realloc, and then change the
                // length and offset
                row->length -= r2->length;
                row->offset += r2->length;
                memmove(&(row->data), &(row->data) + r2->length, row->length);
                r2 = (struct row*)realloc(row, sizeof(struct row) - 1 + row->length);

                if (r2 == NULL)
                {
                    throw -1;
                }

                row = r2;
                ret = row;
            }
            else
            {
                rows.push_front(row);
                count++;
                ret = NULL;
            }
        }
        else if (f->offset == row->offset)
        {
            // If the existing one is at least as long as the new one, we can
            // just copy the new one in and throw it away.
            if (f->length >= row->length)
            {
                memcpy(&(f->data), &(row->data), row->length);
                free(row);
                ret = NULL;
            }
            else
            {
                // In the case where the incoming row is longer than the start one,
                // we need to copy over the first part of the incoming row, and
                // then shuffle everything else around.
                // Hopefully this is rare.
                memcpy(&(f->data), &(row->data), f->length);
                memmove(&(row->data), &(row->data) + f->length, (row->length - f->length));

                row->length -= f->length;
                row->offset += f->length;

                struct row* tmp = (struct row*)realloc(row, sizeof(struct row) - 1 + row->length);

                if (tmp == NULL)
                {
                    throw -1;
                }

                row = tmp;
                ret = row;
            }
        }
    }

    return ret;
}

bool DataCollator::try_add_back(struct DataCollator::row* row)
{
    std::list<struct row*>::reverse_iterator it = rows.rbegin();
    std::list<struct row*>::reverse_iterator end = rows.rend();
    std::list<struct row*>::reverse_iterator prev = end;

    bool ret = false;

    while (it != end)
    {
        // Handle the case of where we're at the end of the list separately.
        if (prev == end)
        {
            // If this one is wholly at the end, just push it onto the end.
            if (((*it)->offset + (*it)->length) <= row->offset)
            {
                rows.push_back(row);
                count++;
                ret = true;
                break;
            }
            // If this one hangs off the end of but extends further in, then
            // push the part that hangs off the end and continue on into the middle
            // with the rest.
            else if (((*it)->offset + (*it)->length) < (row->offset + row->length))
            {
                struct row* r = (struct row*)malloc(sizeof(struct row) - 1 + row->length - (((*it)->offset + (*it)->length) - row->offset));
                r->length = row->length - (((*it)->offset + (*it)->length) - row->offset);
                r->offset = (*it)->offset + (*it)->length;
                row->length -= r->length;
                memcpy(&(r->data), &(row->data) + row->length, r->length);
                rows.push_back(r);
                count++;

                struct row* tmp = (struct row*)realloc(row, sizeof(struct row) - 1 + row->length);

                if (tmp == NULL)
                {
                    throw -1;
                }

                row = tmp;
                ret = false;
            }
            // This one is contained in the last one.
            else if ((row->offset > (*it)->offset) && (((*it)->offset + (*it)->length) >= (row->offset + row->length)))
            {
                memcpy(&((*it)->data) + (row->offset - (*it)->offset), &(row->data), row->length);
                free(row);
                ret = true;
                break;
            }
            // Check to see if the tail of the new one overlaps with this one.
            // If so, copy over the tail, and then realloc the new one
            else if ((row->offset + row->length) > (*it)->offset)
            {
                row->length -= (row->offset + row->length) - (*it)->offset;
                memcpy(&((*it)->data), &(row->data) + row->length, (row->offset + row->length) - (*it)->offset);
                struct row* tmp = (struct row*)realloc(row, sizeof(struct row) - 1 + row->length);

                if (tmp == NULL)
                {
                    throw -1;
                }

                row = tmp;
                ret = row;
            }
            else
            {
                ret = false;
            }
        }
        else
        {
            // If we start at or after the lower row, and end at or before the
            // upper one, we can insert.
            if (row->offset >= ((*it)->offset + (*it)->length))//)
            {
                rows.insert(it.base(), row);
                count++;
                ret = true;
                break;
            }
            // Check to see if we overlap the previous interval.
            else if ((row->offset + row->length) > (*it)->offset)
            {
                // FIrst, see if we need to add a new row in between it and prev
                // to hold the end of row. That is, if row ends after it. Because
                // of this section of code, we can guarantee that it won't end
                // after the start of prev so we don't need to check that anymore.
                if ((row->offset + row->length) > ((*it)->offset + (*it)->length))
                {
                    struct row* r = (struct row*)malloc(sizeof(struct row) - 1 + row->length - (((*it)->offset + (*it)->length) - row->offset));
                    r->length = row->length - (((*it)->offset + (*it)->length) - row->offset);
                    r->offset = (*it)->offset + (*it)->length;
                    row->length -= r->length;
                    memcpy(&(r->data), &(row->data) + row->length, r->length);
                    rows.insert(it.base(), r);
                    count++;
                    ++it;

                    struct row* tmp = (struct row*)realloc(row, sizeof(struct row) - 1 + row->length);

                    if (tmp == NULL)
                    {
                        throw -1;
                    }

                    row = tmp;
                }

                int64_t src_start = MAX((*it)->offset - row->offset, 0);
                int64_t dst_start = MAX(row->offset - (*it)->offset, 0);
                uint64_t len = row->length - src_start;
                row->length = src_start;
                memcpy(&((*it)->data) + dst_start, &(row->data) + row->length, len);

                if (row->length == 0)
                {
                    free(row);
                    ret = true;
                    break;
                }
                else
                {
                    struct row* tmp = (struct row*)realloc(row, sizeof(struct row) - 1 + row->length);

                    if (tmp == NULL)
                    {
                        throw -1;
                    }

                    row = tmp;
                    ret = false;
                }
            }
        }

        prev = it;
        ++it;
    }

    if (!ret)
    {
        free(row);
    }

    return ret;
}

uint64_t DataCollator::truncate_after(int64_t offset)
{
    if (count == 0)
    {
        return 0;
    }

    uint64_t num_bytes = 0;

    std::list<struct row*>::reverse_iterator it = rows.rbegin();
    std::list<struct row*>::reverse_iterator prev = it;
    std::list<struct row*>::reverse_iterator end = rows.rend();
    ++it;

    while (true)
    {
        // Check to see if we start after the offset, because then we can throw
        // away the whole row
        if ((*prev)->offset >= offset)
        {
            num_bytes += (*prev)->length;
            free(*prev);
            rows.erase(it.base());
            count--;
        }
        else
        {
            // Otherwise if we hang out past the offset.
            if (((*prev)->offset + (*prev)->length) > offset)
            {
                num_bytes += ((*prev)->offset + (*prev)->length) - offset;
                (*prev)->length -= ((*prev)->offset + (*prev)->length) - offset;
                struct row* tmp = (struct row*)realloc(*prev, sizeof(struct row) - 1 + (*prev)->length);

                if (tmp == NULL)
                {
                    throw -1;
                }

                (*prev) = tmp;
            }

            // Once we've fixed the hang-off, or if there was none, break.
            break;
        }

        if (it == end)
        {
            break;
        }
        else
        {
            prev = it;
            ++it;
        }
    }

    return num_bytes;
}

uint64_t DataCollator::size()
{
    return count;
}

int64_t DataCollator::get_start()
{
    return start_offset;
}

#endif
