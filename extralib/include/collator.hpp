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
        int32_t offset;
        /// Number of bytes in this data row
        uint32_t length;
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
    DataCollator(void (*_handler)(void* context, uint32_t length, void* data), void* _context);

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
    void add_data(int32_t offset, uint32_t length, void* data);

    /// Used to manually retrieve any data ready for output.
    /// @return NULL if either of the automatic output methods are configured, otherwise it returns
    /// the contents of the node at the head of the list of rows. It may return NULL indicating that
    /// no data is ready for output. This function does not free the row, leaving it up to the user
    /// to free the memory.
    struct row* get_data();

private:

    /// If not NULL, ready data gets written out to this file descriptor.
    FILE* fd;

    /// If not NULL, ready data gets passed to this handler function.
    /// The context argument to the handler function allows for differentiation between
    /// different collators.
    void (*handler)(void* context, uint32_t length, void* data);

    /// Contextual information provided to allow state-based handling of the output
    /// of different collators when using handler functions.
    void* context;

    bool automated_output;

    /// Used to keep strack of where in the stream the next expected data should start.
    int32_t start_offset;

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
    struct row* get_data_p();

    /// A function that will keep only the last N bytes of a row.
    /// @return The newly truncated row.
    /// @param [in] row The row to truncate
    /// @param [in] new_length The number of bytes to keep from the end of a row.
    struct row* truncate_row_end(struct row* row, uint32_t new_length);

    /// A function that will keep only the first N bytes of a row.
    /// @return The newly truncated row.
    /// @param [in] row The row to truncate
    /// @param [in] new_length The number of bytes to keep from the start of a row.
    struct row* truncate_row_start(struct row* row, uint32_t new_length);
    
    /// A function that determines whether two intervals overlap.
    /// @return Whether or not the two intervals specified overlap.
    /// @param [in] a The start point of interval A.
    /// @param [in] aL The length of interval A.
    /// @param [in] b The start point of interval B.
    /// @param [in] bL The length of interval B.
    bool check_overlap(int32_t a, uint32_t aL, int32_t b, uint32_t bL);
};

#endif

