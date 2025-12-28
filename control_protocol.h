#pragma once
#include <stdint.h>

#define CONTROL_MAGIC 0x41444253 // "ADBS"

enum ControlEventType : uint8_t {
    EVENT_TYPE_KEY         = 1,
    EVENT_TYPE_TOUCH_DOWN  = 2,
    EVENT_TYPE_TOUCH_UP    = 3,
    EVENT_TYPE_TOUCH_MOVE  = 4,
    EVENT_TYPE_SCROLL      = 5,
    EVENT_TYPE_BACK        = 6,
    EVENT_TYPE_HOME        = 7
};

#pragma pack(push, 1)
struct ControlPacket {
    uint32_t magic;
    uint8_t  type;
    uint16_t x;
    uint16_t y;
    uint16_t data;
};
#pragma pack(pop)
