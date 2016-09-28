

/**
 * @file
 * a very simple circular buffer FIFO implementation,from ffmpeg
 */

#ifndef HLS_FIFO_H
#define HLS_FIFO_H

#include <stdint.h>

typedef struct _HLSFifoBuffer {
    uint8_t *buffer;
    uint8_t *rptr, *wptr, *end;
    uint32_t rndx, wndx;
} HLSFifoBuffer;

/**
 * Initialize an HLSFifoBuffer.
 * @param size of FIFO
 * @return HLSFifoBuffer or NULL in case of memory allocation failure
 */
HLSFifoBuffer *hls_fifo_alloc(unsigned int size);

/**
 * Free an HLSFifoBuffer.
 * @param f HLSFifoBuffer to free
 */
void hls_fifo_free(HLSFifoBuffer *f);

/**
 * Reset the HLSFifoBuffer to the state right after hls_fifo_alloc, in particular it is emptied.
 * @param f HLSFifoBuffer to reset
 */
void hls_fifo_reset(HLSFifoBuffer *f);

/**
 * Return the amount of data in bytes in the HLSFifoBuffer, that is the
 * amount of data you can read from it.
 * @param f HLSFifoBuffer to read from
 * @return size
 */
int hls_fifo_size(HLSFifoBuffer *f);

/**
 * Return the amount of space in bytes in the HLSFifoBuffer, that is the
 * amount of data you can write into it.
 * @param f HLSFifoBuffer to write into
 * @return size
 */
int hls_fifo_space(HLSFifoBuffer *f);

/**
 * Feed data from an HLSFifoBuffer to a user-supplied callback.
 * @param f HLSFifoBuffer to read from
 * @param buf_size number of bytes to read
 * @param func generic read function
 * @param dest data destination
 */
int hls_fifo_generic_read(HLSFifoBuffer *f, void *dest, int buf_size, void (*func)(void*, void*, int));

/**
 * Feed data from a user-supplied callback to an HLSFifoBuffer.
 * @param f HLSFifoBuffer to write to
 * @param src data source; non-const since it may be used as a
 * modifiable context by the function defined in func
 * @param size number of bytes to write
 * @param func generic write function; the first parameter is src,
 * the second is dest_buf, the third is dest_buf_size.
 * func must return the number of bytes written to dest_buf, or <= 0 to
 * indicate no more data hlsailable to write.
 * If func is NULL, src is interpreted as a simple byte array for source data.
 * @return the number of bytes written to the FIFO
 */
int hls_fifo_generic_write(HLSFifoBuffer *f, void *src, int size, int (*func)(void*, void*, int));

/**
 * Resize an HLSFifoBuffer.
 * In case of reallocation failure, the old FIFO is kept unchanged.
 *
 * @param f HLSFifoBuffer to resize
 * @param size new HLSFifoBuffer size in bytes
 * @return <0 for failure, >=0 otherwise
 */
int hls_fifo_realloc2(HLSFifoBuffer *f, unsigned int size);

/**
 * Enlarge an HLSFifoBuffer.
 * In case of reallocation failure, the old FIFO is kept unchanged.
 * The new fifo size may be larger than the requested size.
 *
 * @param f HLSFifoBuffer to resize
 * @param additional_space the amount of space in bytes to allocate in addition to hls_fifo_size()
 * @return <0 for failure, >=0 otherwise
 */
int hls_fifo_grow(HLSFifoBuffer *f, unsigned int additional_space);

/**
 * Read and discard the specified amount of data from an HLSFifoBuffer.
 * @param f HLSFifoBuffer to read from
 * @param size amount of data to read in bytes
 */
void hls_fifo_drain(HLSFifoBuffer *f, int size);

/**
 * Return a pointer to the data stored in a FIFO buffer at a certain offset.
 * The FIFO buffer is not modified.
 *
 * @param f    HLSFifoBuffer to peek at, f must be non-NULL
 * @param offs an offset in bytes, its absolute value must be less
 *             than the used buffer size or the returned pointer will
 *             point outside to the buffer data.
 *             The used buffer size can be checked with hls_fifo_size().
 */
static inline uint8_t *hls_fifo_peek2(const HLSFifoBuffer *f, int offs)
{
    uint8_t *ptr = f->rptr + offs;
    if (ptr >= f->end)
        ptr = f->buffer + (ptr - f->end);
    else if (ptr < f->buffer)
        ptr = f->end - (f->buffer - ptr);
    return ptr;
}
void hls_fifo_revert(HLSFifoBuffer *f);
#endif /* HLSUTIL_FIFO_H */
