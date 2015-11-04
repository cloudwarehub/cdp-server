#include <stdint.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
    i16 x;
    i16 y;
    u16 width;
    u16 height;
    u8 override;
    u8 viewable;
} cdp_message_create_window_t;