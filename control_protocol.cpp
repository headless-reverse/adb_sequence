#include "control_protocol.h"
#include <QtEndian>
#include <cstring>

QByteArray packetToByteArray(const ControlPacket& packet) {
    QByteArray data;
    data.resize(sizeof(ControlPacket));
    
    ControlPacket* ptr = reinterpret_cast<ControlPacket*>(data.data());
    
    ptr->magic = qToBigEndian<uint32_t>(CONTROL_MAGIC);
    ptr->type  = packet.type;
    ptr->x     = qToBigEndian<uint16_t>(packet.x);
    ptr->y     = qToBigEndian<uint16_t>(packet.y);
    ptr->data  = qToBigEndian<uint16_t>(packet.data);
    
    return data;
}

ControlPacket createTouchPacket(ControlEventType type, uint16_t x, uint16_t y, uint16_t data) {
    ControlPacket packet;
    packet.magic = CONTROL_MAGIC;
    packet.type  = (uint8_t)type;
    packet.x     = x;
    packet.y     = y;
    packet.data  = data;
    return packet;
}

ControlPacket createKeyPacket(uint16_t keyCode) {
    ControlPacket packet;
    packet.magic = CONTROL_MAGIC;
    packet.type  = (uint8_t)EVENT_TYPE_KEY;
    packet.x     = 0;
    packet.y     = 0;
    packet.data  = keyCode;
    return packet;
}
