/******
*  init date: 2013.1.23
*  author: senbai.tao<senbai.tao@amlogic.com>
*  description: this code is part of ffmpeg
******/

#include "curl_common.h"
#include "curl_fifo.h"

Curlfifo *curl_fifo_alloc(unsigned int size)
{
    Curlfifo *f = c_mallocz(sizeof(Curlfifo));
    if (!f) {
        return NULL;
    }
    f->buffer = c_malloc(size);
    f->end = f->buffer + size;
    curl_fifo_reset(f);
    if (!f->buffer) {
        c_freep(&f);
    }
    return f;
}

void curl_fifo_free(Curlfifo *f)
{
    if (f) {
        c_freep(&f->buffer);
        c_free(f);
    }
}

void curl_fifo_reset(Curlfifo *f)
{
    f->wptr = f->rptr = f->buffer;
    f->wndx = f->rndx = 0;
}

int curl_fifo_size(Curlfifo *f)
{
    return (uint32_t)(f->wndx - f->rndx);
}

int curl_fifo_space(Curlfifo *f)
{
    return f->end - f->buffer - curl_fifo_size(f);
}

int curl_fifo_realloc2(Curlfifo *f, unsigned int new_size)
{
    unsigned int old_size = f->end - f->buffer;

    if (old_size < new_size) {
        int len = curl_fifo_size(f);
        Curlfifo *f2 = curl_fifo_alloc(new_size);

        if (!f2) {
            return -1;
        }
        curl_fifo_generic_read(f, f2->buffer, len, NULL);
        f2->wptr += len;
        f2->wndx += len;
        c_free(f->buffer);
        *f = *f2;
        c_free(f2);
    }
    return 0;
}

int curl_fifo_generic_write(Curlfifo *f, void *src, int size, int (*func)(void*, void*, int))
{
    int total = size;
    uint32_t wndx = f->wndx;
    uint8_t *wptr = f->wptr;

    do {
        int len = CURLMIN(f->end - wptr, size);
        if (func) {
            if (func(src, wptr, len) <= 0) {
                break;
            }
        } else {
            memcpy(wptr, src, len);
            src = (uint8_t*)src + len;
        }
        wptr += len;
        if (wptr >= f->end) {
            wptr = f->buffer;
        }
        wndx += len;
        size -= len;
    } while (size > 0);
    f->wndx = wndx;
    f->wptr = wptr;
    return total - size;
}


int curl_fifo_generic_read(Curlfifo *f, void *dest, int buf_size, void (*func)(void*, void*, int))
{
    do {
        int len = CURLMIN(f->end - f->rptr, buf_size);
        if (func) {
            func(dest, f->rptr, len);
        } else {
            memcpy(dest, f->rptr, len);
            dest = (uint8_t*)dest + len;
        }
        curl_fifo_drain(f, len);
        buf_size -= len;
    } while (buf_size > 0);
    return 0;
}

void curl_fifo_drain(Curlfifo *f, int size)
{
    f->rptr += size;
    if (f->rptr >= f->end) {
        f->rptr -= f->end - f->buffer;
    }
    f->rndx += size;
}
