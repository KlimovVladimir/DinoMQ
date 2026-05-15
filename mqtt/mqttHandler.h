#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include "log.h"

#define PACKET_PARSED      1
#define NOT_FULL_PACKET   -1
#define PACKET_MALFORMED  -2

#define MQTT_OK            0
#define MQTT_CLOSE        -1


/* Control packet types */
#define RESERVED       0x00U
#define CONNECT        0x10U
#define CONNACK        0x20U
#define PUBLISH        0x30U
#define PUBACK         0x40U
#define PUBREC         0x50U
#define PUBREL         0x60U
#define PUBCOMP        0x70U
#define SUBSCRIBE      0x80U
#define SUBACK         0x90U
#define UNSUBSCRIBE    0xA0U
#define UNSUBACK       0xB0U
#define PINGREQ        0xC0U
#define PINGRESP       0xD0U
#define DISCONNECT     0xE0U

/* Connect Return code values */
#define CONN_ACCEPTED              0x00U
#define CONN_REFUSED_PROTOCOL      0x01U
#define CONN_ACCEPTED_IDENTIFIER   0x02U
#define CONN_ACCEPTED_UNAVALIABLE  0x03U
#define CONN_ACCEPTED_BADUSERPASS  0x04U
#define CONN_ACCEPTED_NOTAUTH      0x05U

typedef struct FixedHeader {
    uint8_t packetType;
    int remainingLengthVal;
    int remainingLengthSize;
} FixedHeader;

typedef struct MQTTMessage {
    FixedHeader fHeader;
    int totalLength;
} MQTTMessage;

struct MQTTClient;

int parseMQTTPacket(struct MQTTClient *client);
int handleMQTTMessage(struct MQTTClient *client);
int handleMQTTConnect(struct MQTTClient *client);
int handleMQTTPingReq(struct MQTTClient *client);

#endif //MQTT_HANDLER_H