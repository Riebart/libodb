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
    ///
    /// This provides flexibility in allowing the output to be written to either a file, a socket,
    /// or stdin of a process started using popen and acquiring the write (stdin) descriptor.
    DataCollator(FILE* _fd);

    /// Constructor that instructs the collator to write output to the specified handler function.
    /// @param [in] _handler The function that takes in a data row (length and data pointer) and
    /// does something with it.
    /// @param [in,out] _context An opaque binary row that is passed to the handler function
    /// when it is called allowing handler functions to behave differently based on the collator
    /// that is calling them.
    ///
    /// Useful for when more flexibility is needed, but automatic output is still required or
    /// desired.
    DataCollator(void (*_handler)(void* context, uint64_t length, void* data), void* _context);

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

private:
    /// Initialize the member variables.
    void init();

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

    /// Used to keep strack of where in the stream the next expected data should start.
    int64_t start_offset;

    /// Holds the number of rows inserted into the list so that we aren't calling the
    /// O(n) .size() method of the STL list.
    uint64_t count;

    /// Keeps track of the list of rows allowing for space to exist between rows, representing missing data.
    std::list<struct row*> rows;

    /// Attempts to add the new row to the front of the list, to fill in the first gap.
    /// @param [in] row The row to add.
    void add_back(struct row* row);

    /// Attempts to add the new row to the front of the list, to fill in the first gap.
    /// @return Whether or not it was successful in adding the item to the front.
    /// @param [in] row The row to add.
    bool try_add_front(struct row* row);

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

    /// A function that will keep only the last N bytes of a row.
    /// @return The newly truncated row.
    /// @param [in] row The row to truncate
    /// @param [in] new_length The number of bytes to keep from the end of a row.
    struct row* truncate_row_end(struct row* row, uint64_t new_length);

    /// A function that will keep only the first N bytes of a row.
    /// @return The newly truncated row.
    /// @param [in] row The row to truncate
    /// @param [in] new_length The number of bytes to keep from the start of a row.
    struct row* truncate_row_start(struct row* row, uint64_t new_length);
};

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
        free(*it);
        it = rows.erase(it);
        count--;
    }
}

void DataCollator::init()
{
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

DataCollator::DataCollator(FILE* _fd)
{
    init();

    this->fd = _fd;
    automated_output = true;
}

DataCollator::DataCollator(void (*_handler)(void* context, uint64_t length, void* data), void* _context)
{
    init();

    this->handler = _handler;
    this->context = _context;
    automated_output = true;
}

void DataCollator::add_data(int64_t offset, uint64_t length, void* data)
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

struct DataCollator::row* DataCollator::truncate_row_end(struct DataCollator::row* row, uint64_t new_length)
{
    struct row* r2 = (struct row*)malloc(sizeof(struct row) + new_length - 1);

    if (r2 == NULL)
    {
        throw -1;
    }

    r2->offset += row->length - new_length;
    r2->length = new_length;
    memcpy(&(r2->data), &(row->data) + row->length - new_length, r2->length);
    free(row);

    return r2;
}

struct DataCollator::row* DataCollator::truncate_row_start(struct DataCollator::row* row, uint64_t new_length)
{
    struct row* r2 = (struct row*)malloc(sizeof(struct row) + new_length - 1);

    if (r2 == NULL)
    {
        throw -1;
    }

    r2->offset = row->offset;
    r2->length = new_length;
    memcpy(&(r2->data), &(row->data), r2->length);
    free(row);

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

    bool ret = false;

    if (start_offset > row->offset)
    {
        if (start_offset >= (int64_t)((row->offset + row->length)))
        {
            // If we're here, then there is nothing to add, so just throw away
            // the row and return false;
            free(row);
            return false;
        }

        row = truncate_row_end(row, row->length - (start_offset - row->offset));
    }

    if (count == 0)
    {
        rows.push_front(row);
        count++;
        ret = true;
    }
    else
    {
        struct row* f = rows.front();

        // Now check to see if we actually are inserting here. This happens if
        // thew new row starts before the first non-NULL row. Also check for
        // overlap.
        if (f->offset > row->offset)
        {
            rows.push_front(row);
            count++;
            ret = true;
        }
    }

    return ret;
}

void DataCollator::add_back(struct DataCollator::row* row)
{
    std::list<struct row*>::reverse_iterator it = rows.rbegin();
    std::list<struct row*>::reverse_iterator end = rows.rend();
    std::list<struct row*>::reverse_iterator prev = end;

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

                struct row* tmp = (struct row*)realloc(row, sizeof(struct row) + row->length - 1);

                if (tmp == NULL)
                {
                    throw -1;
                }

                row = tmp;
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

                    struct row* tmp = (struct row*)realloc(row, sizeof(struct row) + row->length - 1);

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
                    break;
                }
                else
                {
                    struct row* tmp = (struct row*)realloc(row, sizeof(struct row) + row->length - 1);

                    if (tmp == NULL)
                    {
                        throw -1;
                    }

                    row = tmp;
                }
            }
        }

        prev = it;
        ++it;
    }
}

uint64_t DataCollator::size()
{
    return count;
}

#endif
