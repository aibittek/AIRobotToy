#ifndef _WIFI_H_
#define _WIFI_H_

#include <stdint.h>
#include <ringbuffer.h>

#define _ASSERT(e) \
    if (!(e)) printf("assert fail in file:%s, line:%d\r\n", __FILE__, __LINE__)

// 结构体的定义
typedef struct {
    void        (*begin)            (int32_t speed);
	uint8_t     (*connect_ap)       (const char *, const char *);
    uint8_t     (*connect_server)   (const char *, const char *, int16_t);
    uint32_t    (*send)             (void *, int32_t);
    uint32_t    (*recv)             (void *, int32_t, uint32_t);
    uint32_t    (*close)            (void);
    
    ring_buffer_t                   buffer;
}_net_t;
extern _net_t net;

#endif
