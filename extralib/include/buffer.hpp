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

/// Header file containing the declaration and definition of buffered file IO routines.
/// The methods and structures in this file allow for explicit buffering of file
///IO in memory in order to maximise throughput to/from disk or other locations.
/// @file buffer.hpp

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common.hpp"

#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif

/// Struct that holds all of the context necessary for the buffered IO.
/// The same structure is used for both reading and writing, with no indication
///as to how it was created contained in the structure.
/// @todo Mark the buffer context as read or write, and throw errors when the user uses one for the wrong purpose up?
struct file_buffer
{
    /// Pointer to the file that we are buffering.
    FILE* fp;

    /// Byte array that the buffered content is stored in
    uint8_t* buffer;

    /// Position in the byte array that we are currently reading from/writing to
    uint32_t position;

    /// Size of 'remaining' buffer
    /// In the case of reading, this represents how much data is left to be read
    ///from the buffer before a new disk read is incurred. In the case of writing
    ///this holds how many bytes can be written to the buffer before incurring a
    ///write to the underlying file.
    uint32_t size;

    /// Maximum capacity of the buffer/
    uint32_t buf_size;
};

/// Handles cleanup of the buffering context.
/// It is important to note that this does not close the file handle but only
///cleans up the buffering context including freeing the struct and the buffer.
/// @param[in] fb The context to clean up
void fb_destroy(struct file_buffer* fb)
{
    free(fb->buffer);
    free(fb);
}

/// Initiatlizes a new read-buffering context from a file pointer.
/// @attention It is assumed that the file pointer given here was opened
///appropriately (that is, read, and on Windows in binary or text mode as
///appropriate). No verification is done to ensure this.
/// @param[in] fp File pointer to use as the underlying file.
/// @param[in] num_bytes Size of the memory buffer array.
/// @return A buffering context initialized with given parameters
struct file_buffer* fb_read_init(FILE* fp, uint32_t num_bytes = 1048576)
{
    if (fp == NULL)
    {
        return NULL;
    }

    struct file_buffer* fb;
    SAFE_MALLOC(struct file_buffer*, fb, sizeof(struct file_buffer));

    if (fb == NULL)
    {
        return NULL;
    }

    fb->buf_size = num_bytes;
    fb->buffer = (uint8_t*)(malloc(num_bytes));
    if (fb->buffer == NULL)
    {
        free(fb);
        return NULL;
    }

    fb->fp = fp;

    fb->size = fread(fb->buffer, 1, fb->buf_size, fp);

    //error doing initial read, other than a short file
    if (fb->size < fb->buf_size && ferror(fb->fp) != 0)
    {
        return NULL;
    }
    fb->position = 0;

    return fb;
}

/// Initiatlizes a new read-buffering context from a file name.
/// This function opens the file from its name and passes the pointer it gets
///to fb_read_init(FILE*, uint32_t).
/// @note This opens the file in binary mode, which has implications on Windows
///platforms.
/// @attention No verification is done that the file actually exists, so you
/// may end up with a null pointer passed along.
/// @param[in] fname File name to open.
/// @param[in] num_bytes Size of the memory buffer array.
/// @return A buffering context initialized with given parameters
struct file_buffer* fb_read_init(char* fname, uint32_t num_bytes = 1048576)
{
    FILE* fp = fopen(fname, "rb");
    return fb_read_init(fp, num_bytes);
}

/// Initiatlizes a new write-buffering context from a file pointer.
/// @attention It is assumed that the file pointer given here was opened
///appropriately (that is, read, and on Windows in binary or text mode as
///appropriate). No verification is done to ensure this.
/// @param[in] fp File pointer to use as the underlying file.
/// @param[in] num_bytes Size of the memory buffer array.
/// @return A buffering context initialized with given parameters
struct file_buffer* fb_write_init(FILE* fp, uint32_t num_bytes = 1048576)
{
    if (fp == NULL)
    {
        return NULL;
    }

    struct file_buffer* fb;
    SAFE_MALLOC(struct file_buffer*, fb, sizeof(struct file_buffer));

    if (fb == NULL)
    {
        return NULL;
    }

    fb->buf_size = num_bytes;
    fb->buffer = (uint8_t*)(malloc(num_bytes));
    memset(fb->buffer, 0, num_bytes);
    if (fb->buffer == NULL)
    {
        free(fb);
        return NULL;
    }

    fb->fp = fp;

    fb->size = num_bytes;
    fb->position = 0;

    return fb;
}

/// Initiatlizes a new write-buffering context from a file name.
/// This function opens the file from its name and passes the pointer it gets
///to fb_read_init(FILE*, uint32_t).
/// @note This opens the file in binary mode, which has implications on Windows
///platforms.
/// @attention No verification is done that the file actually exists, so you
/// may end up with a null pointer passed along.
/// @param[in] fname File name to open.
/// @param[in] num_bytes Size of the memory buffer array.
/// @return A buffering context initialized with given parameters
struct file_buffer* fb_write_init(char* fname, uint32_t num_bytes = 1048576)
{
    FILE* fp = fopen(fname, "wb");
    return fb_write_init(fp, num_bytes);
}

/// Write data through the buffer and flush to the file if necessary.
/// @param[in] fb The buffer context to write to.
/// @param[in] src A pointer to the data that is being written.
/// @param[in] num_bytes Number of bytes of data to write from src to the buffer.
/// @return Number of bytes written to the buffer/flushed to the file.
uint32_t fb_write(struct file_buffer* fb, void* src, uint32_t num_bytes)
{
    uint32_t numput;

    // If this write won't fill the buffer
    if (num_bytes <= (fb->size - fb->position))
    {
        memcpy(fb->buffer + fb->position, src, num_bytes);
        fb->position += num_bytes;
        numput = num_bytes;

        //should this write be delayed?
        if (fb->position == fb->size)
        {
            if (fwrite(fb->buffer, fb->buf_size, 1, fb->fp) > 0)
            {
                fb->position = 0;
            }
        }
    }
    // Otherwise
    else
    {
        uint32_t min;
        numput = 0;

        while (numput < num_bytes)
        {
            min = MIN((fb->size - fb->position), (uint32_t)(num_bytes - numput));
            memcpy(fb->buffer + fb->position, ((uint8_t*)src) + numput, min);
            fb->position += min;

            if (fb->position == fb->size)
            {
                if (fwrite(fb->buffer, fb->buf_size, 1, fb->fp) > 0)
                {
                    fb->position = 0;
                }
                else
                {
                    min = 0;
                }
            }

            numput += min;
        }
    }

    return numput;
}

/// Flush the contents, if any, of the write buffer to the file
/// @param[in] fb The buffering context to flush.
/// @return Number of bytes flushed to the file.
uint32_t fb_write_flush(struct file_buffer* fb)
{
    uint32_t numput = 0;

    if (fb->position > 0)
    {
        numput = fwrite(fb->buffer, fb->position, 1, fb->fp);

        if (numput > 0)
        {
            fb->position = 0;
        }
    }

    return numput;
}

/// Read data from the buffer/prime the buffer if the read is larger.
/// @param[in] fb Buffering context to read from.
/// @param[in,out] dest Pointer to the destination where the data will be stored.
/// @param[in] num_bytes Number of bytes to read from the buffering context
///into the destination.
/// @return Number of bytes read from the buffering context into the destination.
/// @attention There is an assumption that the destination points to enough
///allocated memory to store num_bytes read from the buffer/file.
/// @bug Does not accurately determine end of file. Should use feof() like fb_read_line
uint32_t fb_read(struct file_buffer* fb, void* dest, uint32_t num_bytes)
{
    uint32_t numread;

    // If we've got enough bytes in the buffer...
    if (num_bytes <= (fb->size - fb->position))
    {
        memcpy(dest, fb->buffer + fb->position, num_bytes);
        fb->position += num_bytes;
        numread = num_bytes;

        // If this read has left the buffer empty, refill it.
        //TF - I think this should be removed/delayed to when the data is needed
        if (fb->position == fb->size)
        {
            fb->size = fread(fb->buffer, 1, fb->buf_size, fb->fp);
            fb->position = 0;
        }
    }
    // Otherwise
    else
    {
        uint32_t min;
        numread = 0;

        while (numread < num_bytes)
        {
            min = MIN((fb->size - fb->position), (uint32_t)(num_bytes - numread));
            memcpy(((uint8_t*)dest) + numread, fb->buffer + fb->position, min);
            fb->position += min;
            numread += min;

            if (fb->position == fb->size)
            {
                fb->size = fread(fb->buffer, 1, fb->buf_size, fb->fp);
                fb->position = 0;

                if (fb->size == 0)
                {
                    return numread;
                }
            }
        }
    }

    return numread;
}

/// Read a line (terminated by a newline) from the buffering context up to a maximum length
/// This function reads until it hits a newline character, or until num_bytes of
///data are read from the buffer/file, whichever comes first.
/// @param[in] fb Buffering context to read from.
/// @param[in,out] dest Pointer to the destination where the data will be stored.
/// @param[in] num_bytes Number of bytes to read from the buffering context
///into the destination.
/// @return Number of bytes read from the buffering context into the destination.
/// @attention There is an assumption that the destination points to enough
///allocated memory to store num_bytes read from the buffer/file.
/// @bug This function is not thoroughly tested.
int32_t fb_read_line(struct file_buffer* fb, void* dest, uint32_t num_bytes)
{
    uint32_t numread=0;
    char * str_dest = (char*)dest;

    if (fb->size <= fb->position && (feof(fb->fp) || ferror(fb->fp)))
    {
        return -1;
    }

    while (numread < num_bytes && fb->buffer[fb->position] != '\n')
    {
        str_dest[numread] = fb->buffer[fb->position];

        numread++;
        fb->position++;

        //expand the buffer, if necessary
        if (fb->size <= fb->position)
        {
            fb->size = fread(fb->buffer, 1, fb->buf_size, fb->fp);
            fb->position = 0;

            if (fb->size == 0)
            {
                str_dest[numread] = '\0';
                return numread;
            }
        }
    }

    //advance past newline
    fb->position++;
    str_dest[numread] = '\0';

    return numread+1;
}

#endif
