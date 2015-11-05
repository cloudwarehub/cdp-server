#include <stdint.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;

enum CDP_MESSAGE {
    CDP_MESSAGE_CREATE_WINDOW,
    CDP_MESSAGE_DESTROY_WINDOW,
    CDP_MESSAGE_SHOW_WINDOW,
    CDP_MESSAGE_HIDE_WINDOW,
    CDP_MESSAGE_WINDOW_FRAME
};

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
    u16 _pad1;
} cdp_message_create_window_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
} cdp_message_destroy_window_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
} cdp_message_show_window_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
} cdp_message_hide_window_t;

typedef struct {
    u8 msgtype;
    u8 _pad;
    u16 sequence;
    u32 length;
    u32 window;
} cdp_message_window_frame_t;
