#include "mqttHandler.h"
#include "epollHandler.h"

int parseMQTTPacket(struct MQTTClient *client) {
    int multiplier = 1;
    int value = 0;
    int size = 0;
    int i = 1;
    uint8_t encoded;

    do {
        if (i >= client->in_len)
            return NOT_FULL_PACKET;

        if (size == 4)
            return PACKET_MALFORMED;

        encoded = client->inbuf[i++];

        value += (encoded & 0x7F) * multiplier;
        multiplier *= 0x80;
        size++;
    } while (encoded & 0x80);

    int totalLength = 1 + size + value;

    if (totalLength > client->in_len)
        return NOT_FULL_PACKET;

    client->msg.fHeader.packetType = client->inbuf[0];
    client->msg.fHeader.remainingLengthVal = value;
    client->msg.fHeader.remainingLengthSize = size;
    client->msg.totalLength = totalLength;

    return PACKET_PARSED;
}

void handleMQTTConnect(struct MQTTClient *client) {
    size_t pos = 1 + client->msg.fHeader.remainingLengthSize;
    uint16_t protoLength = (client->inbuf[pos] << 8) | client->inbuf[pos + 1];
    pos += 2;
    
    char protocolName[5];
    strncpy(protocolName, &client->inbuf[pos], 4);
    protocolName[4] = '\0';
    pos += protoLength;

    if (strcmp(protocolName, "MQTT") != 0) {

    }

    uint8_t version = client->inbuf[pos++];
    uint8_t connectFlags = client->inbuf[pos++];
    uint16_t keepAlive = (client->inbuf[pos] << 8) | client->inbuf[pos + 1];
    pos += 2;

    uint16_t clientIDLength = (client->inbuf[pos] << 8) | client->inbuf[pos + 1];
    pos += 2;
    char clientID[clientIDLength + 1];
    strncpy(clientID, &client->inbuf[pos], clientIDLength);
    clientID[clientIDLength] = '\0';
    pos += clientIDLength;


    uint8_t connack[] = {CONNACK, 0x02, 0x00, 0x00};

    if (queuePacket(client, connack, sizeof(connack)) == -1) {
        close(client->fd);
        return;
    }

    enableClientWrite(client);
}

void handleMQTTPingReq(struct MQTTClient *client) {
    uint8_t pingResp[] = {PINGRESP, 0x00};

    if (queuePacket(client, pingResp, sizeof(pingResp)) == -1) {
        close(client->fd);
        return;
    }

    enableClientWrite(client);
}


void handleMQTTMessage(struct MQTTClient *client) {
    uint8_t command = client->msg.fHeader.packetType & 0xF0;
    uint8_t flags = client->msg.fHeader.packetType & 0x0F;

    switch (command)
    {
        case RESERVED:
            logPrint(LOG_INFO, "[%s] Handle RESERVED: fd=%d\n", __func__, client->fd);
            break;

        case CONNECT:
            logPrint(LOG_INFO, "[%s] Handle CONNECT: fd=%d\n", __func__, client->fd);
            handleMQTTConnect(client);
            break;

        case CONNACK:
            logPrint(LOG_INFO, "[%s] Handle CONNACK: fd=%d\n", __func__, client->fd);
            break;

        case PUBLISH:
            logPrint(LOG_INFO, "[%s] Handle PUBLISH: fd=%d\n", __func__, client->fd);
            break;

        case PUBACK:
            logPrint(LOG_INFO, "[%s] Handle PUBACK: fd=%d\n", __func__, client->fd);
            break;

        case PUBREC:
            logPrint(LOG_INFO, "[%s] Handle PUBREC: fd=%d\n", __func__, client->fd);
            break;

        case PUBREL:
            logPrint(LOG_INFO, "[%s] Handle PUBREL: fd=%d\n", __func__, client->fd);
            break;

        case PUBCOMP:
            logPrint(LOG_INFO, "[%s] Handle PUBCOMP: fd=%d\n", __func__, client->fd);
            break;

        case SUBSCRIBE:
            logPrint(LOG_INFO, "[%s] Handle SUBSCRIBE: fd=%d\n", __func__, client->fd);
            break;

        case SUBACK:
            logPrint(LOG_INFO, "[%s] Handle SUBACK: fd=%d\n", __func__, client->fd);
            break;

        case UNSUBSCRIBE:
            logPrint(LOG_INFO, "[%s] Handle UNSUBSCRIBE: fd=%d\n", __func__, client->fd);
            break;

        case UNSUBACK:
            logPrint(LOG_INFO, "[%s] Handle UNSUBACK: fd=%d\n", __func__, client->fd);
            break;

        case PINGREQ:
            logPrint(LOG_INFO, "[%s] Handle PINGREQ: fd=%d\n", __func__, client->fd);
            handleMQTTPingReq(client);
            break;

        case PINGRESP:
            logPrint(LOG_INFO, "[%s] Handle PINGRESP: fd=%d\n", __func__, client->fd);
            break;

        case DISCONNECT:
            logPrint(LOG_INFO, "[%s] Handle DISCONNECT: fd=%d\n", __func__, client->fd);
            break;

        default:
            logPrint(LOG_INFO, "[%s] Unknown packet: 0x%02X fd=%d\n", __func__, command, client->fd);
            break;
    }
}