#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <stdio.h>
#include <stdint.h>

#define MIN(x,y) (x<y?x:y)

struct file_buffer
{
    FILE* fp;
    uint8_t* buffer;
    uint32_t position;
    uint32_t size;
    uint32_t read_size;
};

struct file_buffer* fb_read_init(FILE* fp, uint32_t num_bytes)
{
    struct file_buffer* fb = (struct file_buffer*)malloc(sizeof(struct file_buffer));

    if (fb == NULL)
        return NULL;

    fb->read_size = num_bytes;
    fb->buffer = (uint8_t*)(malloc(num_bytes));
    fb->fp = fp;

    fb->size = fread(fb->buffer, 1, fb->read_size, fp);
    fb->position = 0;

    return fb;
}

uint16_t fb_read(struct file_buffer* fb, void* dest, uint16_t num_bytes)
{
    uint16_t numread;

    // If we've got enough bytes in the buffer...
    if (num_bytes <= (fb->size - fb->position))
    {
        memcpy(dest, fb->buffer + fb->position, num_bytes);
        fb->position += num_bytes;
        numread = num_bytes;
    }
    // Otherwise
    else
    {
        uint16_t min;
        numread = 0;

        while (numread < num_bytes)
        {
            min = MIN((fb->size - fb->position), (uint32_t)(num_bytes - numread));
            memcpy(((uint8_t*)dest) + numread, fb->buffer + fb->position, min);
            fb->position += min;
            numread += min;

            if (fb->position >= fb->size)
            {
                fb->size = fread(fb->buffer, 1, fb->read_size, fb->fp);
                fb->position = 0;

                if (fb->size == 0)
                    return numread;
            }
        }
    }

    // If this read has left the buffer empty, refill it.
    if (fb->position == fb->size)
    {
        fb->size = fread(fb->buffer, 1, fb->read_size, fb->fp);
        fb->position = 0;
    }

    return numread;
}

#endif