#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#include <stdint.h>
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <iostream>
#include <stdio.h>
#include <string.h>

typedef struct RingBuffer{
	uint8_t*m_buffer;
    bool    m_mlocked;
    size_t  m_writer;
    size_t *m_readers;
    size_t  m_size;
    int 	nb_readers; // number of reader threads
} RingBuffer;


size_t rb_get_write_space(RingBuffer *rb);
size_t rb_get_read_space(RingBuffer *rb, int reader_idx);
size_t rb_read(RingBuffer *rb, uint8_t* target, int reader_idx, size_t nb);
size_t rb_read_overlap(RingBuffer *rb, uint8_t* target, int reader_idx, size_t nb, float overlap);
size_t rb_write(RingBuffer* rb, const uint8_t* source, size_t nb);
size_t rb_zero(RingBuffer* rb, size_t nb);
int    rb_create(RingBuffer* rb, size_t length, int r);
void   print_buffer_stats(RingBuffer* rb);
size_t rb_get_max_read_space(RingBuffer* rb);
void   rb_free(RingBuffer* rb);
int    add_reader(RingBuffer* rb);
size_t rb_skip(RingBuffer* rb, size_t to_skip, int reader);
void   rb_reset(RingBuffer* rb);
#endif /* RINGBUFFER_H */