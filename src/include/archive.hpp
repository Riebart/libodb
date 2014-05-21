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

/// Header file for Archive objects definition details, including children (AppendOnlyFile).
/// @file archive.hpp

#include "dll.hpp"

#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "comparator.hpp"

/// The parent class for other archival objects.
///
/// This class is the virtual parent class that the rest of ODB makes use of
///when archiving data to disk.
class LIBODB_API Archive
{
public:
    /// The function used to write a piece of data according to the rules of the object defines.
    /// @param[in] rawdata Pointer to the data to be written
    /// @param[in] datalen The number of bytes of data to write.
    /// @return Whether or not the write was successful.
    virtual bool write(void* rawdata, uint64_t datalen) = 0;

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
///
/// This implements an append-only archival system that aims to simply dump data
///to disk in a format that is efficiently written to platter disks. The format
///makes use of a pair of files, a .dat and a .ind, which together allow for the
///storage of the data. The .dat file contains the data itself, and the .ind file
///contains the offsets in the .dat file that correspond to the beginnings of
///pieces of data. The .ind file is only necessary (but is always included) for
///variable width data. Both files are stored in binary format, with the .ind
///file containing 64-bit integers in the endianness of the architecture of the
///host that wrote them. No endianness indicators are placed in the .ind file.
class LIBODB_API AppendOnlyFile : public Archive
{
public:
    /// Standard destructor
    ~AppendOnlyFile();

    /// Standard constructor
    /// @param[in] base_filename The base file name, to which .dat and .ind are
    ///appended for the files on disk.
    /// @param[in] append If the files exist already, should they be opened for
    ///appending, or should their contents be overwritten?
    AppendOnlyFile(char* base_filename, bool append);

    /// The function used to write a piece of data
    /// @param[in] rawdata Pointer to the data to be written
    /// @param[in] datalen The number of bytes of data to write.
    /// @return Whether or not the write was successful.
    virtual bool write(void* rawdata, uint64_t datalen);

private:
    /// File name of the file that holds the raw data.
    char* data_name;

    /// File name of the file that holds the index.
    char* index_name;

    ///  File pointer of the file that holds the raw data.
    FILE* data;

    /// File pointer of the file that holds the index.
    FILE* index;

    /// Offset into the .dat file. This is the value written to the .ind file.
    uint64_t offset;
};

#endif
