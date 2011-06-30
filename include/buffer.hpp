#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif

// This will be used to ensure that the buffer is never smaller than 4096 bytes.
// This helps with page-aligned memory, and should be large enough for sectors on a hard disk.
#ifndef MAX
#define MAX(x, y) (x > y ? x : y)
#endif

struct file_buffer
{
    FILE* fp;
    uint8_t* buffer;
    uint32_t position;
    uint32_t size;
    uint32_t buf_size;
};

void fb_destroy(struct file_buffer* fb)
{
    free(fb->buffer);
    free(fb);
}

struct file_buffer* fb_read_init(FILE* fp, uint32_t num_bytes)
{
    struct file_buffer* fb = (struct file_buffer*)malloc(sizeof(struct file_buffer));

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
    fb->position = 0;

    return fb;
}

struct file_buffer* fb_read_init(char* fname, uint32_t num_bytes)
{
    FILE* fp = fopen(fname, "rb");
    return fb_read_init(fp, num_bytes);
}

struct file_buffer* fb_write_init(FILE* fp, uint32_t num_bytes)
{
    struct file_buffer* fb = (struct file_buffer*)malloc(sizeof(struct file_buffer));

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

    fb->size = num_bytes;
    fb->position = 0;

    return fb;
}

struct file_buffer* fb_write_init(char* fname, uint32_t num_bytes)
{
    FILE* fp = fopen(fname, "wb");
    return fb_write_init(fp, num_bytes);
}

uint32_t fb_write(struct file_buffer* fb, void* src, uint32_t num_bytes)
{
    uint32_t numput;

    // If this write won't fill the buffer
    if (num_bytes <= (fb->size - fb->position))
    {
        memcpy(fb->buffer + fb->position, src, num_bytes);
        fb->position += num_bytes;
        numput = num_bytes;

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

// uint32_t fb_write(struct file_buffer* fb, void* src, uint32_t num_bytes, bool flush)
// {
//     uint32_t numput = fb_write(fb, src, num_bytes);
//
//     // If we have explicitly flushed, make sure we write the buffer out, if it has anything in it.
//     if ((flush) && (fb->position > 0))
//     {
//         if (fwrite(fb->buffer, fb->position, 1, fb->fp) > 0)
//         {
//             fb->position = 0;
//         }
//     }
//
//     return numput;
// }

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

//untested!
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
