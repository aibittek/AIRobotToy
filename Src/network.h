#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stm32f4xx.h>
struct network {
    uint8_t (*connect_ap)(const char *ssid, const char *passwd);
};

#endif
