#include <stdlib.h>
#include "log.h"
#include "ringbuffer.h"

/**
 * @file
 * Implementation of ring buffer functions.
 */

bool ring_buffer_init(ring_buffer_t *buffer, ring_buffer_size_t size) {
    if ((size & (size - 1)) != 0) {
        LOG(EERROR, "RING_BUFFER_SIZE must be a power of two");
        return false;
    }
    buffer->total_size = size;
    buffer->mask = size - 1;
    buffer->tail_index = 0;
    buffer->head_index = 0;
    buffer->buffer = malloc(size);
    if (buffer->buffer) return true;
    else return false;
}

void ring_buffer_queue(ring_buffer_t *buffer, char data) {
  /* Is buffer full? */
  if(ring_buffer_is_full(buffer)) {
    /* Is going to overwrite the oldest byte */
    /* Increase tail index */
    buffer->tail_index = ((buffer->tail_index + 1) & buffer->mask);
  }

  /* Place data in buffer */
  buffer->buffer[buffer->head_index] = data;
  buffer->head_index = ((buffer->head_index + 1) & buffer->mask);
}

void ring_buffer_queue_arr(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size) {
  /* Add bytes; one by one */
  ring_buffer_size_t i;
  for(i = 0; i < size; i++) {
    ring_buffer_queue(buffer, data[i]);
  }
}

uint8_t ring_buffer_dequeue(ring_buffer_t *buffer, char *data) {
  if(ring_buffer_is_empty(buffer)) {
    /* No items */
    return 0;
  }
  
  *data = buffer->buffer[buffer->tail_index];
  buffer->tail_index = ((buffer->tail_index + 1) & buffer->mask);
  return 1;
}

ring_buffer_size_t ring_buffer_dequeue_arr(ring_buffer_t *buffer, char *data, ring_buffer_size_t len) {
  if(ring_buffer_is_empty(buffer)) {
    /* No items */
    return 0;
  }

  char *data_ptr = data;
  ring_buffer_size_t cnt = 0;
  while((cnt < len) && ring_buffer_dequeue(buffer, data_ptr)) {
    cnt++;
    data_ptr++;
  }
  return cnt;
}

uint8_t ring_buffer_peek(ring_buffer_t *buffer, char *data, ring_buffer_size_t index) {
  if(index >= ring_buffer_num_items(buffer)) {
    /* No items at index */
    return 0;
  }
  
  /* Add index to pointer */
  ring_buffer_size_t data_index = ((buffer->tail_index + index) & buffer->mask);
  *data = buffer->buffer[data_index];
  return 1;
}

void ring_buffer_uninit(ring_buffer_t *buffer) {
    if (buffer->buffer) {
        free(buffer->buffer);
        buffer->buffer = NULL;
        buffer->total_size = 0;
        buffer->head_index = buffer->tail_index = 0;
    }
}

extern inline uint8_t ring_buffer_is_empty(ring_buffer_t *buffer);
extern inline uint8_t ring_buffer_is_full(ring_buffer_t *buffer);
extern inline ring_buffer_size_t ring_buffer_num_items(ring_buffer_t *buffer);

