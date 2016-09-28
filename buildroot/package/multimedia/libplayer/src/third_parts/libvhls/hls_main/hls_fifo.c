#define LOG_NDEBUG 0
#define LOG_TAG "HLS_FIFO"

#include "hls_fifo.h"
#include "hls_utils.h"

#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif




HLSFifoBuffer *hls_fifo_alloc(unsigned int size)
{
    HLSFifoBuffer *f= malloc(sizeof(HLSFifoBuffer));
    if (!f){
        LOGE("failed to malloc fifo buffer context\n");
        return NULL;

    }
    f->buffer = malloc(size);
    f->end = f->buffer + size;
    hls_fifo_reset(f);
    if (!f->buffer){
        LOGE("failed to malloc fifo buffer\n");
        in_freepointer(&f);

    }
    memset(f->buffer,0,size);
    LOGV("Succeed to allocate fifo\n");
    return f;
}

void hls_fifo_free(HLSFifoBuffer *f)
{
    if (f) {
        in_freepointer(&f->buffer);
        free(f);
    }
}

void hls_fifo_reset(HLSFifoBuffer *f)
{
    f->wptr = f->rptr = f->buffer;
    f->wndx = f->rndx = 0;
}

int hls_fifo_size(HLSFifoBuffer *f)
{
    return (uint32_t)(f->wndx - f->rndx);
}

int hls_fifo_space(HLSFifoBuffer *f)
{
    return f->end - f->buffer - hls_fifo_size(f);
}

int hls_fifo_realloc2(HLSFifoBuffer *f, unsigned int new_size)
{
    unsigned int old_size = f->end - f->buffer;

    if (old_size < new_size) {
        int len = hls_fifo_size(f);
        HLSFifoBuffer *f2 = hls_fifo_alloc(new_size);

        if (!f2)
            return -1;
        hls_fifo_generic_read(f, f2->buffer, len, NULL);
        f2->wptr += len;
        f2->wndx += len;
        free(f->buffer);
        *f = *f2;
        free(f2);
    }
    return 0;
}

int hls_fifo_grow(HLSFifoBuffer *f, unsigned int size)
{
    unsigned int old_size = f->end - f->buffer;
    if(size + (unsigned)hls_fifo_size(f) < size)
        return -1;

    size += hls_fifo_size(f);

    if (old_size < size)
        return hls_fifo_realloc2(f, HLSMAX(size, 2*size));
    return 0;
}

// src must NOT be const as it can be a context for func that may need updating (like a pointer or byte counter)
int hls_fifo_generic_write(HLSFifoBuffer *f, void *src, int size, int (*func)(void*, void*, int))
{
    int total = size;
    uint32_t wndx= f->wndx;
    uint8_t *wptr= f->wptr;

    do {
        int len = HLSMIN(f->end - wptr, size);
        if (func) {
            if (func(src, wptr, len) <= 0)
                break;
        } else {
            memcpy(wptr, src, len);
            src = (uint8_t*)src + len;
        }
// Write memory barrier needed for SMP here in theory
        wptr += len;
        if (wptr >= f->end)
            wptr = f->buffer;
        wndx += len;
        size -= len;
    } while (size > 0);
    f->wndx= wndx;
    f->wptr= wptr;
    return total - size;
}


int hls_fifo_generic_read(HLSFifoBuffer *f, void *dest, int buf_size, void (*func)(void*, void*, int))
{
// Read memory barrier needed for SMP here in theory
    do {
        int len = HLSMIN(f->end - f->rptr, buf_size);
        if(func) func(dest, f->rptr, len);
        else{
            memcpy(dest, f->rptr, len);
            dest = (uint8_t*)dest + len;
        }
// memory barrier needed for SMP here in theory
        hls_fifo_drain(f, len);
        buf_size -= len;
    } while (buf_size > 0);
    return 0;
}

/** Discard data from the FIFO. */
void hls_fifo_drain(HLSFifoBuffer *f, int size)
{
    f->rptr += size;
    if (f->rptr >= f->end)
        f->rptr -= f->end - f->buffer;
    f->rndx += size;
}
void hls_fifo_revert(HLSFifoBuffer *f){
    f->rptr = f->buffer;
    f->rndx = 0;    
}