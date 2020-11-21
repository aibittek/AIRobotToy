#ifndef _MW_MQTT_H_
#define _MW_MQTT_H_

#include <stdint.h>

#define MQTT_BUFFER_SIZE    1024

typedef struct
{
	uint8_t     rxbuf[MQTT_BUFFER_SIZE];
	uint8_t     txbuf[MQTT_BUFFER_SIZE];
    uint32_t    rxlen;
    uint32_t    txlen;
    
	void        (*begin)                (void);
	uint8_t     (*connect)              (char *client_id,char *username,char *password);
	uint8_t     (*subscribe_topic)      (char *topic, uint8_t qos, uint8_t whether);
	uint8_t     (*publish_data)         (char *topic, char *message, uint8_t qos);
	void        (*send_heart)           (void);
	void        (*disconnect)           (void);
    void        (*end)                  (void);
}_mqtt_t;
extern _mqtt_t mqtt;

#endif

