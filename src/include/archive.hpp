/* MPL2.0 HEADER START
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2013 Michael Himbeault and Travis Friesen
 *
 */

#ifndef ARCHIVER_HPP
#define ARCHIVER_HPP

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "comparator.hpp"

/// The parent class for other archival objects.
///
/// This class is the virtual parent class that the rest of ODB makes use of
///when archiving data to disk.
class Archive
{
public:
    /// The function used to write a piece of data according to the rules of the object defines.
    /// @param[in] rawdata Pointer to the data to be written
    /// @param[in] datalen The number of bytes of data to write.
    /// @return Whether or not the write was successful.
    virtual bool write(void* rawdata, uint32_t datalen) = 0;
    
    /// Function that sets the constraint on whether or not data will be written to the archive.
    /// @param[in] _cond Condition that embodies the logic that deteermines if
    ///a piece of data gets written.
    virtual void set_constraint(Condition* _cond)
    {
        this->cond = _cond;
    };

protected:
    /// The condition used to determine if a data should be written.
    Condition* cond;
};

/// An implementation of an append-only file archiver.

class AppendOnlyFile : public Archive
{
public:
    ~AppendOnlyFile();
    AppendOnlyFile(char* base_filename, bool append);
    virtual bool write(void* rawdata, uint32_t datalen);

private:
    char* data_name;
    char* index_name;
    FILE* data;
    FILE* index;
    uint64_t offset;
};

#endif

///